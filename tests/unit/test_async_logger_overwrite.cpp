#include <cstddef>
#include <cstdio>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/full_queue_policy.h>
#include <cms/log/level_filter.h>
#include <cms/log/printf_log.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/log/utc_offset_formatter.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>

#include "test.h"

namespace {

struct ClockState {
    cms::log::Timestamp current = 0;
    std::size_t calls = 0;
};

struct CountingClock {
    explicit CountingClock(ClockState& state) noexcept : state_(&state) {}

    cms::log::Timestamp nowMilliseconds() noexcept {
        ++state_->calls;
        return state_->current;
    }

private:
    ClockState* state_;
};

struct MutexState {
    std::size_t locks = 0;
    std::size_t unlocks = 0;
    bool locked = false;
};

struct CountingMutex {
    explicit CountingMutex(MutexState& state) noexcept : state_(&state) {}

    void lock() noexcept {
        ++state_->locks;
        state_->locked = true;
    }

    void unlock() noexcept {
        ++state_->unlocks;
        state_->locked = false;
    }

private:
    MutexState* state_;
};

struct SinkState {
    cms::StaticString<512> lines[32];
    std::size_t writes = 0;
    bool failed = false;
    MutexState* queueMutex = nullptr;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::StringView value) noexcept {
        if (state_->queueMutex != nullptr && state_->queueMutex->locked) {
            state_->failed = true;
            return;
        }
        if (state_->writes >= 32
            || state_->lines[state_->writes].assign(value).status
                != cms::Status::ok) {
            state_->failed = true;
            return;
        }
        ++state_->writes;
    }

private:
    SinkState* state_;
};

void checkBytes(cms::StringView actual, cms::StringView expected) {
    CMS_TEST_REQUIRE(actual.size() == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

void checkLine(
    const SinkState& state,
    std::size_t index,
    cms::StringView expected) {
    CMS_TEST_REQUIRE(index < state.writes);
    checkBytes(state.lines[index].view(), expected);
}

struct CountingFormatter {
    static constexpr std::size_t maxOverhead =
        cms::log::PlainFormatter::maxOverhead;
    static int calls;

    static cms::WriteResult format(
        const cms::log::Record& record,
        cms::StringBuffer output) noexcept {
        ++calls;
        return cms::log::PlainFormatter::format(record, output);
    }
};

int CountingFormatter::calls = 0;

using ExternalMutex = cms::sync::MutexRef<CountingMutex>;

template<class Formatter, class LevelFilter, class FullQueuePolicy>
using TestLogger = cms::log::AsyncLogger<
    64,
    1,
    CountingClock,
    CapturingSink,
    cms::sync::NullMutex,
    Formatter,
    LevelFilter,
    FullQueuePolicy>;

} // namespace

int main() {
    {
        using Logger = cms::log::AsyncLogger<
            64, 2, CountingClock, CapturingSink, ExternalMutex>;
        ClockState clock;
        SinkState sink;
        MutexState mutexState;
        CountingMutex mutex(mutexState);
        sink.queueMutex = &mutexState;
        Logger logger{
            CountingClock(clock),
            CapturingSink(sink),
            ExternalMutex(mutex)};

        clock.current = 10;
        CMS_TEST_CHECK(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        clock.current = 20;
        CMS_TEST_CHECK(logger.log(cms::log::Level::warning, "B")
            == cms::Status::ok);
        CMS_TEST_CHECK(logger.full());
        CMS_TEST_CHECK(logger.wouldLog(cms::log::Level::critical));
        const std::size_t locksBefore = mutexState.locks;
        const std::size_t unlocksBefore = mutexState.unlocks;
        clock.current = 30;
        CMS_TEST_CHECK(logger.log(cms::log::Level::critical, "C")
            == cms::Status::no_space);
        CMS_TEST_CHECK(clock.calls == 3);
        CMS_TEST_CHECK(mutexState.locks == locksBefore + 1);
        CMS_TEST_CHECK(mutexState.unlocks == unlocksBefore + 1);
        CMS_TEST_CHECK(!mutexState.locked);
        CMS_TEST_CHECK(logger.pending() == 2);
        CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
        CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[10] [INFO] A\n");
        checkLine(sink, 1, "[20] [WARNING] B\n");
        CMS_TEST_CHECK(!sink.failed);
    }

    {
        using Logger = cms::log::AsyncLogger<
            64,
            1,
            CountingClock,
            CapturingSink,
            ExternalMutex,
            cms::log::PlainFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        MutexState mutexState;
        CountingMutex mutex(mutexState);
        sink.queueMutex = &mutexState;
        Logger logger{
            CountingClock(clock),
            CapturingSink(sink),
            ExternalMutex(mutex)};

        clock.current = 11;
        CMS_TEST_CHECK(logger.log(cms::log::Level::info, "old")
            == cms::Status::ok);
        const std::size_t locksBefore = mutexState.locks;
        const std::size_t unlocksBefore = mutexState.unlocks;
        clock.current = 22;
        CMS_TEST_CHECK(logger.log(cms::log::Level::error, "new")
            == cms::Status::ok);
        CMS_TEST_CHECK(mutexState.locks == locksBefore + 1);
        CMS_TEST_CHECK(mutexState.unlocks == unlocksBefore + 1);
        CMS_TEST_CHECK(!mutexState.locked);
        CMS_TEST_CHECK(logger.pending() == 1);
        CMS_TEST_CHECK(logger.capacity() == 1);
        CMS_TEST_CHECK(logger.full());
        CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[22] [ERROR] new\n");
        CMS_TEST_CHECK(clock.calls == 2);
        CMS_TEST_CHECK(!sink.failed);
    }

    {
        using Logger = cms::log::AsyncLogger<
            64,
            2,
            CountingClock,
            CapturingSink,
            cms::sync::NullMutex,
            cms::log::PlainFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        clock.current = 1;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        clock.current = 2;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::warning, "B")
            == cms::Status::ok);
        clock.current = 3;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::error, "C")
            == cms::Status::ok);
        CMS_TEST_CHECK(logger.pending() == 2);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[2] [WARNING] B\n");
        clock.current = 4;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::critical, "D")
            == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 1, "[3] [ERROR] C\n");
        checkLine(sink, 2, "[4] [CRITICAL] D\n");
        CMS_TEST_CHECK(logger.empty());
    }

    {
        using Logger = cms::log::AsyncLogger<
            64,
            3,
            CountingClock,
            CapturingSink,
            cms::sync::NullMutex,
            cms::log::PlainFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        const cms::StringView messages[] = {"A", "B", "C", "D", "E"};
        for (std::size_t index = 0; index < 5; ++index) {
            clock.current = index + 1;
            CMS_TEST_REQUIRE(logger.log(
                cms::log::Level::info,
                messages[index]) == cms::Status::ok);
        }
        CMS_TEST_CHECK(logger.pending() == 3);
        CMS_TEST_CHECK(logger.full());
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[3] [INFO] C\n");
        checkLine(sink, 1, "[4] [INFO] D\n");
        checkLine(sink, 2, "[5] [INFO] E\n");
        CMS_TEST_CHECK(logger.empty());
    }

    {
        using Logger = cms::log::AsyncLogger<
            64,
            2,
            CountingClock,
            CapturingSink,
            ExternalMutex,
            cms::log::PlainFormatter,
            cms::log::RuntimeLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        MutexState mutexState;
        CountingMutex mutex(mutexState);
        sink.queueMutex = &mutexState;
        Logger logger{
            CountingClock(clock),
            CapturingSink(sink),
            ExternalMutex(mutex)};

        logger.setMinLevel(cms::log::Level::trace);
        clock.current = 1;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        clock.current = 2;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::warning, "B")
            == cms::Status::ok);
        CMS_TEST_CHECK(logger.full());

        logger.setMinLevel(cms::log::Level::warning);
        CMS_TEST_CHECK(!logger.wouldLog(cms::log::Level::info));
        const std::size_t filteredClockCalls = clock.calls;
        const std::size_t filteredLocks = mutexState.locks;
        CMS_TEST_CHECK(logger.log(cms::log::Level::info, "filtered")
            == cms::Status::ok);
        CMS_TEST_CHECK(clock.calls == filteredClockCalls);
        CMS_TEST_CHECK(mutexState.locks == filteredLocks);
        CMS_TEST_CHECK(mutexState.unlocks == filteredLocks);

        logger.setLoggingEnabled(false);
        CMS_TEST_CHECK(!logger.wouldLog(cms::log::Level::critical));
        CMS_TEST_CHECK(logger.log(cms::log::Level::critical, "disabled")
            == cms::Status::ok);
        CMS_TEST_CHECK(clock.calls == filteredClockCalls);
        CMS_TEST_CHECK(mutexState.locks == filteredLocks);
        CMS_TEST_CHECK(mutexState.unlocks == filteredLocks);

        logger.setLoggingEnabled(true);
        logger.setMinLevel(cms::log::Level::trace);
        CMS_TEST_CHECK(logger.wouldLog(cms::log::Level::info));
        CMS_TEST_CHECK(logger.full());
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[1] [INFO] A\n");
        checkLine(sink, 1, "[2] [WARNING] B\n");
        CMS_TEST_CHECK(!sink.failed);
    }

    {
        using RejectLogger = TestLogger<
            cms::log::PlainFormatter,
            cms::log::NoLevelFilter,
            cms::log::RejectOnFull>;
        using OverwriteLogger = TestLogger<
            cms::log::PlainFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;

        ClockState rejectClock;
        SinkState rejectSink;
        RejectLogger reject{
            CountingClock(rejectClock),
            CapturingSink(rejectSink)};
        rejectClock.current = 1;
        CMS_TEST_REQUIRE(cms::log::logf(
            reject, cms::log::Level::info, "value=%d", 1)
            == cms::Status::ok);
        rejectClock.current = 2;
        CMS_TEST_CHECK(cms::log::logf(
            reject, cms::log::Level::info, "value=%d", 2)
            == cms::Status::no_space);
        CMS_TEST_REQUIRE(reject.drainOne() == cms::Status::ok);
        checkLine(rejectSink, 0, "[1] [INFO] value=1\n");

        ClockState overwriteClock;
        SinkState overwriteSink;
        OverwriteLogger overwrite{
            CountingClock(overwriteClock),
            CapturingSink(overwriteSink)};
        overwriteClock.current = 3;
        CMS_TEST_REQUIRE(cms::log::logf(
            overwrite, cms::log::Level::info, "value=%d", 3)
            == cms::Status::ok);
        overwriteClock.current = 4;
        CMS_TEST_CHECK(cms::log::logf(
            overwrite, cms::log::Level::error, "value=%d", 4)
            == cms::Status::ok);
        CMS_TEST_REQUIRE(overwrite.drainOne() == cms::Status::ok);
        checkLine(overwriteSink, 0, "[4] [ERROR] value=4\n");
    }

    {
        using Logger = TestLogger<
            cms::log::AnsiFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        clock.current = 1;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        clock.current = 2;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::error, "B")
            == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[2] \033[31m[ERROR]\033[0m B\n");
    }

    {
        using Logger = TestLogger<
            cms::log::RuntimeAnsiFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        clock.current = 2;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::warning, "B")
            == cms::Status::ok);
        logger.setUseColor(false);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(sink, 0, "[2] [WARNING] B\n");
    }

    {
        using Logger = TestLogger<
            cms::log::StyledAnsiFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "old")
            == cms::Status::ok);
        clock.current = 3;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::error, "[NET] FAIL")
            == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(
            sink,
            0,
            "[3] \033[31m[ERROR]\033[0m \033[95m[NET]\033[0m "
            "\033[1;91mFAIL\033[0m\n");
    }

    {
        using Formatter = cms::log::UtcOffsetFormatter<
            cms::log::RuntimeStyledAnsiFormatter>;
        using Logger = cms::log::AsyncLogger<
            64,
            1,
            CountingClock,
            CapturingSink,
            ExternalMutex,
            Formatter,
            cms::log::RuntimeLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        MutexState mutexState;
        CountingMutex mutex(mutexState);
        sink.queueMutex = &mutexState;
        Logger logger{
            CountingClock(clock),
            CapturingSink(sink),
            ExternalMutex(mutex)};
        logger.setMinLevel(cms::log::Level::warning);
        logger.setUseColor(true);
        CMS_TEST_REQUIRE(logger.log(
            cms::log::Level::warning,
            "[OLD] FAIL") == cms::Status::ok);
        CMS_TEST_REQUIRE(cms::log::logf(
            logger,
            cms::log::Level::error,
            "[NET] FAIL code=%d",
            7) == cms::Status::ok);
        CMS_TEST_CHECK(logger.pending() == 1);
        CMS_TEST_CHECK(logger.full());

        logger.setUseColor(false);
        CMS_TEST_CHECK(logger.setUtcOffsetMinutes(540) == cms::Status::ok);
        const std::size_t filteredClockCalls = clock.calls;
        const std::size_t filteredLocks = mutexState.locks;
        CMS_TEST_CHECK(logger.log(cms::log::Level::debug, "filtered")
            == cms::Status::ok);
        CMS_TEST_CHECK(clock.calls == filteredClockCalls);
        CMS_TEST_CHECK(mutexState.locks == filteredLocks);
        CMS_TEST_CHECK(mutexState.unlocks == filteredLocks);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        checkLine(
            sink,
            0,
            "[09:00:00] [ERROR] [NET] FAIL code=7\n");
        CMS_TEST_CHECK(!sink.failed);
    }

    {
        using Logger = TestLogger<
            CountingFormatter,
            cms::log::NoLevelFilter,
            cms::log::OverwriteOldestOnFull>;
        ClockState clock;
        SinkState sink;
        Logger logger{CountingClock(clock), CapturingSink(sink)};
        CountingFormatter::calls = 0;
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "A")
            == cms::Status::ok);
        CMS_TEST_REQUIRE(logger.log(cms::log::Level::info, "B")
            == cms::Status::ok);
        CMS_TEST_CHECK(CountingFormatter::calls == 0);
        CMS_TEST_CHECK(sink.writes == 0);
        CMS_TEST_REQUIRE(logger.drainOne() == cms::Status::ok);
        CMS_TEST_CHECK(CountingFormatter::calls == 1);
        CMS_TEST_CHECK(sink.writes == 1);
    }

    using PlainLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex>;
    using AnsiLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::AnsiFormatter>;
    using RuntimeAnsiLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeAnsiFormatter>;
    using RuntimeLevelLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::PlainFormatter, cms::log::RuntimeLevelFilter>;
    using StyledLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::StyledAnsiFormatter>;
    using RuntimeStyledLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeStyledAnsiFormatter>;
    using RuntimeStyledLevelLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeStyledAnsiFormatter,
        cms::log::RuntimeLevelFilter>;
    using UtcPlainLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::UtcOffsetFormatter<>>;
    using UtcRuntimeAnsiLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::UtcOffsetFormatter<cms::log::RuntimeAnsiFormatter>>;
    using UtcRuntimeStyledLevelLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::UtcOffsetFormatter<
            cms::log::RuntimeStyledAnsiFormatter>,
        cms::log::RuntimeLevelFilter>;
    using ExplicitRejectLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::PlainFormatter, cms::log::NoLevelFilter,
        cms::log::RejectOnFull>;
    using OverwriteLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::PlainFormatter, cms::log::NoLevelFilter,
        cms::log::OverwriteOldestOnFull>;
    using Record = cms::log::StaticRecord<64>;
    using Queue = cms::StaticQueue<Record, 8>;

    CMS_TEST_CHECK(sizeof(PlainLogger) == sizeof(ExplicitRejectLogger));
    CMS_TEST_CHECK(sizeof(PlainLogger) == sizeof(OverwriteLogger));
    std::printf("sizeof(existing plain logger)=%zu\n", sizeof(PlainLogger));
    std::printf("sizeof(existing ANSI logger)=%zu\n", sizeof(AnsiLogger));
    std::printf("sizeof(existing runtime ANSI logger)=%zu\n",
        sizeof(RuntimeAnsiLogger));
    std::printf("sizeof(existing runtime level logger)=%zu\n",
        sizeof(RuntimeLevelLogger));
    std::printf("sizeof(existing styled logger)=%zu\n",
        sizeof(StyledLogger));
    std::printf("sizeof(existing runtime styled logger)=%zu\n",
        sizeof(RuntimeStyledLogger));
    std::printf("sizeof(existing runtime styled + level logger)=%zu\n",
        sizeof(RuntimeStyledLevelLogger));
    std::printf("sizeof(UTC offset plain logger)=%zu\n",
        sizeof(UtcPlainLogger));
    std::printf("sizeof(UTC offset runtime ANSI logger)=%zu\n",
        sizeof(UtcRuntimeAnsiLogger));
    std::printf("sizeof(UTC offset runtime styled + level logger)=%zu\n",
        sizeof(UtcRuntimeStyledLevelLogger));
    std::printf("sizeof(explicit reject logger)=%zu\n",
        sizeof(ExplicitRejectLogger));
    std::printf("sizeof(overwrite-oldest logger)=%zu\n",
        sizeof(OverwriteLogger));
    std::printf("sizeof(StaticRecord<64>)=%zu\n", sizeof(Record));
    std::printf("sizeof(StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(Queue));

    return cms::test::finish();
}
