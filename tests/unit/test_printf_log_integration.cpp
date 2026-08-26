#include <cstddef>
#include <cstdio>

#include <cms/log/ansi_formatter.h>
#include <cms/log/level_filter.h>
#include <cms/log/printf_log.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
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

struct SinkState {
    cms::StaticString<512> lines[16];
    std::size_t writes = 0;
    bool failed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::StringView value) noexcept {
        if (state_->writes >= 16
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

template<class Formatter>
void checkStaticFormatter(
    cms::StringView expected,
    cms::log::Level level) {
    using Logger = cms::log::AsyncLogger<
        64,
        1,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        Formatter>;

    ClockState clockState;
    SinkState sinkState;
    clockState.current = 1;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};
    CMS_TEST_CHECK(cms::log::logf(
        logger, level, "[NET] FAIL code=%d", 5) == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(sinkState, 0, expected);
    CMS_TEST_CHECK(clockState.calls == 1);
    CMS_TEST_CHECK(!sinkState.failed);
}

} // namespace

int main() {
    checkStaticFormatter<cms::log::AnsiFormatter>(
        "[1] \033[31m[ERROR]\033[0m [NET] FAIL code=5\n",
        cms::log::Level::error);
    checkStaticFormatter<cms::log::StyledAnsiFormatter>(
        "[1] \033[31m[ERROR]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mFAIL\033[0m code=5\n",
        cms::log::Level::error);

    using RuntimeAnsiLogger = cms::log::AsyncLogger<
        64,
        2,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::RuntimeAnsiFormatter>;
    ClockState runtimeAnsiClock;
    SinkState runtimeAnsiSink;
    RuntimeAnsiLogger runtimeAnsi{
        CountingClock(runtimeAnsiClock),
        CapturingSink(runtimeAnsiSink)};
    runtimeAnsiClock.current = 2;
    CMS_TEST_CHECK(cms::log::logf(
        runtimeAnsi,
        cms::log::Level::warning,
        "value=%d",
        7) == cms::Status::ok);
    CMS_TEST_CHECK(runtimeAnsi.drainOne() == cms::Status::ok);
    checkLine(
        runtimeAnsiSink,
        0,
        "[2] \033[33m[WARNING]\033[0m value=7\n");
    runtimeAnsi.setUseColor(false);
    runtimeAnsiClock.current = 3;
    CMS_TEST_CHECK(cms::log::logf(
        runtimeAnsi,
        cms::log::Level::error,
        "value=%d",
        8) == cms::Status::ok);
    CMS_TEST_CHECK(runtimeAnsi.drainOne() == cms::Status::ok);
    checkLine(runtimeAnsiSink, 1, "[3] [ERROR] value=8\n");

    using ExternalMutex = cms::sync::MutexRef<CountingMutex>;
    using RuntimeStyledLogger = cms::log::AsyncLogger<
        64,
        2,
        CountingClock,
        CapturingSink,
        ExternalMutex,
        cms::log::RuntimeStyledAnsiFormatter,
        cms::log::RuntimeLevelFilter>;
    ClockState styledClock;
    SinkState styledSink;
    MutexState mutexState;
    CountingMutex mutex(mutexState);
    RuntimeStyledLogger styled{
        CountingClock(styledClock),
        CapturingSink(styledSink),
        ExternalMutex(mutex)};

    styled.setMinLevel(cms::log::Level::warning);
    styled.setUseColor(true);
    CMS_TEST_CHECK(!styled.wouldLog(cms::log::Level::debug));
    CMS_TEST_CHECK(styled.wouldLog(cms::log::Level::warning));
    CMS_TEST_CHECK(styled.wouldLog(static_cast<cms::log::Level>(0xFF)));
    const std::size_t filteredLocks = mutexState.locks;
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::debug,
        "%s",
        "this filtered message is far longer than the logger buffer and "
        "must never be formatted") == cms::Status::ok);
    CMS_TEST_CHECK(styledClock.calls == 0);
    CMS_TEST_CHECK(mutexState.locks == filteredLocks);
    CMS_TEST_CHECK(mutexState.unlocks == filteredLocks);
    CMS_TEST_CHECK(!mutexState.locked);

    styledClock.current = 4;
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::warning,
        "[NET] FAIL code=%d",
        5) == cms::Status::ok);
    CMS_TEST_CHECK(styled.drainOne() == cms::Status::ok);
    checkLine(
        styledSink,
        0,
        "[4] \033[33m[WARNING]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mFAIL\033[0m code=5\n");

    styled.setUseColor(false);
    styledClock.current = 5;
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::error,
        "[NET] FAIL code=%d",
        6) == cms::Status::ok);
    CMS_TEST_CHECK(styled.drainOne() == cms::Status::ok);
    checkLine(styledSink, 1, "[5] [ERROR] [NET] FAIL code=6\n");

    styled.setLoggingEnabled(false);
    CMS_TEST_CHECK(!styled.wouldLog(cms::log::Level::critical));
    CMS_TEST_CHECK(!styled.wouldLog(static_cast<cms::log::Level>(0xFF)));
    const std::size_t disabledClockCalls = styledClock.calls;
    const std::size_t disabledLocks = mutexState.locks;
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::critical,
        static_cast<const char*>(nullptr)) == cms::Status::ok);
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::critical,
        "%s",
        "this disabled message is far longer than the logger buffer and "
        "must never be formatted") == cms::Status::ok);
    CMS_TEST_CHECK(styledClock.calls == disabledClockCalls);
    CMS_TEST_CHECK(mutexState.locks == disabledLocks);
    CMS_TEST_CHECK(mutexState.unlocks == disabledLocks);
    CMS_TEST_CHECK(!mutexState.locked);

    styled.setLoggingEnabled(true);
    styled.setMinLevel(cms::log::Level::trace);
    CMS_TEST_CHECK(styled.wouldLog(static_cast<cms::log::Level>(0xFF)));
    styledClock.current = 6;
    CMS_TEST_CHECK(cms::log::logf(
        styled, cms::log::Level::info, "first=%d", 1)
        == cms::Status::ok);
    styledClock.current = 7;
    CMS_TEST_CHECK(cms::log::logf(
        styled, cms::log::Level::error, "second=%d", 2)
        == cms::Status::ok);
    CMS_TEST_CHECK(styled.pending() == 2);

    styled.setMinLevel(cms::log::Level::critical);
    const std::size_t fullFilteredClock = styledClock.calls;
    const std::size_t fullFilteredLocks = mutexState.locks;
    CMS_TEST_CHECK(cms::log::logf(
        styled,
        cms::log::Level::warning,
        "%s",
        "filtered even though the queue is full") == cms::Status::ok);
    CMS_TEST_CHECK(styledClock.calls == fullFilteredClock);
    CMS_TEST_CHECK(mutexState.locks == fullFilteredLocks);
    CMS_TEST_CHECK(mutexState.unlocks == fullFilteredLocks);

    styledClock.current = 8;
    const std::size_t fullLocks = mutexState.locks;
    CMS_TEST_CHECK(cms::log::logf(
        styled, cms::log::Level::critical, "third=%d", 3)
        == cms::Status::no_space);
    CMS_TEST_CHECK(styledClock.calls == fullFilteredClock + 1);
    CMS_TEST_CHECK(mutexState.locks == fullLocks + 1);
    CMS_TEST_CHECK(mutexState.unlocks == fullLocks + 1);
    CMS_TEST_CHECK(!mutexState.locked);
    CMS_TEST_CHECK(styled.pending() == 2);
    CMS_TEST_CHECK(styled.drainOne() == cms::Status::ok);
    CMS_TEST_CHECK(styled.drainOne() == cms::Status::ok);
    checkLine(styledSink, 2, "[6] [INFO] first=1\n");
    checkLine(styledSink, 3, "[7] [ERROR] second=2\n");
    CMS_TEST_CHECK(styled.empty());
    CMS_TEST_CHECK(!styledSink.failed);

    using PlainLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex>;
    using AnsiLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::AnsiFormatter>;
    using RuntimeAnsiSizeLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeAnsiFormatter>;
    using RuntimeLevelLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::PlainFormatter, cms::log::RuntimeLevelFilter>;
    using StyledLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::StyledAnsiFormatter>;
    using RuntimeStyledSizeLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeStyledAnsiFormatter>;
    using RuntimeStyledLevelLogger = cms::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::sync::NullMutex,
        cms::log::RuntimeStyledAnsiFormatter, cms::log::RuntimeLevelFilter>;
    using Record = cms::log::StaticRecord<64>;
    using Queue = cms::StaticQueue<Record, 8>;

    std::printf("sizeof(existing plain logger)=%zu\n", sizeof(PlainLogger));
    std::printf("sizeof(existing ANSI logger)=%zu\n", sizeof(AnsiLogger));
    std::printf("sizeof(existing runtime ANSI logger)=%zu\n",
        sizeof(RuntimeAnsiSizeLogger));
    std::printf("sizeof(existing runtime level logger)=%zu\n",
        sizeof(RuntimeLevelLogger));
    std::printf("sizeof(styled logger)=%zu\n", sizeof(StyledLogger));
    std::printf("sizeof(runtime styled logger)=%zu\n",
        sizeof(RuntimeStyledSizeLogger));
    std::printf("sizeof(runtime styled + level logger)=%zu\n",
        sizeof(RuntimeStyledLevelLogger));
    std::printf("sizeof(StaticRecord<64>)=%zu\n", sizeof(Record));
    std::printf("sizeof(StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(Queue));
    std::printf("printf local scratch for MessageBytes=64: %zu bytes\n",
        std::size_t{64});

    return cms::test::finish();
}
