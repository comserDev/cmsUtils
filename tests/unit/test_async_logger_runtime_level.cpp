#include <cstddef>
#include <cstdio>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/mutex_ref.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

struct CountingMutex {
    void lock() noexcept {
        ++locks;
        locked = true;
    }

    void unlock() noexcept {
        ++unlocks;
        locked = false;
    }

    int locks = 0;
    int unlocks = 0;
    bool locked = false;
};

struct ClockState {
    cms::util::log::Timestamp current = 0;
    std::size_t calls = 0;
};

struct CountingClock {
    explicit CountingClock(ClockState& state) noexcept
        : state_(&state) {}

    cms::util::log::Timestamp nowMilliseconds() noexcept {
        ++state_->calls;
        return state_->current;
    }

private:
    ClockState* state_;
};

struct SinkState {
    cms::util::StaticString<128> lines[16];
    std::size_t writes = 0;
    CountingMutex* queueMutex = nullptr;
    bool observedLockedMutex = false;
    bool captureFailed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept
        : state_(&state) {}

    cms::util::Status write(cms::util::StringView text) noexcept {
        if (state_->queueMutex != nullptr && state_->queueMutex->locked) {
            state_->observedLockedMutex = true;
        }
        if (state_->writes >= 16) {
            state_->captureFailed = true;
            return cms::util::Status::io_error;
        }
        if (state_->lines[state_->writes].assign(text).status
            != cms::util::Status::ok) {
            state_->captureFailed = true;
        }
        ++state_->writes;
        return state_->captureFailed
            ? cms::util::Status::io_error
            : cms::util::Status::ok;
    }

private:
    SinkState* state_;
};

void checkBytes(cms::util::StringView actual, cms::util::StringView expected) {
    CMS_TEST_REQUIRE(actual.size() == expected.size());
    if (!expected.empty()) {
        CMS_TEST_REQUIRE(actual.data() != nullptr);
        CMS_TEST_REQUIRE(expected.data() != nullptr);
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

void checkLine(
    const SinkState& state,
    std::size_t index,
    cms::util::StringView expected) {
    CMS_TEST_REQUIRE(index < state.writes);
    checkBytes(state.lines[index].view(), expected);
}

} // namespace

int main() {
    using RuntimeLevelLogger = cms::util::log::AsyncLogger<
        16,
        3,
        CountingClock,
        CapturingSink,
        cms::util::sync::MutexRef<CountingMutex>,
        cms::util::log::PlainFormatter,
        cms::util::log::RuntimeLevelFilter>;

    ClockState clockState;
    CountingMutex queueMutex;
    SinkState sinkState;
    sinkState.queueMutex = &queueMutex;
    RuntimeLevelLogger logger{
        CountingClock(clockState),
        CapturingSink(sinkState),
        cms::util::sync::MutexRef<CountingMutex>(queueMutex)};

    CMS_TEST_CHECK(logger.loggingEnabled());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::debug);
    const int locksBeforeTrace = queueMutex.locks;
    const int unlocksBeforeTrace = queueMutex.unlocks;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::trace, "filtered")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);
    CMS_TEST_CHECK(queueMutex.locks == locksBeforeTrace);
    CMS_TEST_CHECK(queueMutex.unlocks == unlocksBeforeTrace);
    CMS_TEST_CHECK(logger.pending() == 0);

    logger.setMinLevel(cms::util::log::Level::warning);
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::warning);
    char oversized[16] = {};
    const int locksBeforeOversized = queueMutex.locks;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::debug,
        cms::util::StringView(oversized, sizeof(oversized))) == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);
    CMS_TEST_CHECK(queueMutex.locks == locksBeforeOversized);
    CMS_TEST_CHECK(logger.pending() == 0);

    clockState.current = 10;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "queued")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 1);
    CMS_TEST_CHECK(logger.pending() == 1);
    logger.setMinLevel(cms::util::log::Level::critical);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 0, "[10] [WARNING] queued\n");

    logger.setMinLevel(cms::util::log::Level::error);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "never")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 1);
    logger.setMinLevel(cms::util::log::Level::trace);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);
    CMS_TEST_CHECK(sinkState.writes == 1);

    logger.setMinLevel(cms::util::log::Level::warning);
    clockState.current = 11;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "first")
        == cms::util::Status::ok);
    clockState.current = 12;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "second")
        == cms::util::Status::ok);
    clockState.current = 13;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "third")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.full());

    const int locksBeforeFullFilter = queueMutex.locks;
    const int unlocksBeforeFullFilter = queueMutex.unlocks;
    const std::size_t callsBeforeFullFilter = clockState.calls;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "ignored")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == callsBeforeFullFilter);
    CMS_TEST_CHECK(queueMutex.locks == locksBeforeFullFilter);
    CMS_TEST_CHECK(queueMutex.unlocks == unlocksBeforeFullFilter);
    CMS_TEST_CHECK(logger.pending() == 3);

    logger.setLoggingEnabled(false);
    CMS_TEST_CHECK(!logger.loggingEnabled());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::warning);
    const int locksBeforeDisabled = queueMutex.locks;
    const int unlocksBeforeDisabled = queueMutex.unlocks;
    const std::size_t callsBeforeDisabled = clockState.calls;
    const cms::util::log::Level invalid = static_cast<cms::util::log::Level>(0xFF);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "disabled")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.log(invalid, "disabled-invalid")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::critical,
        cms::util::StringView(oversized, sizeof(oversized))) == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == callsBeforeDisabled);
    CMS_TEST_CHECK(queueMutex.locks == locksBeforeDisabled);
    CMS_TEST_CHECK(queueMutex.unlocks == unlocksBeforeDisabled);
    CMS_TEST_CHECK(logger.pending() == 3);
    logger.setLoggingEnabled(true);
    CMS_TEST_CHECK(logger.loggingEnabled());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::warning);

    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 1, "[11] [WARNING] first\n");
    checkLine(sinkState, 2, "[12] [ERROR] second\n");
    checkLine(sinkState, 3, "[13] [CRITICAL] third\n");
    CMS_TEST_CHECK(logger.empty());

    logger.setMinLevel(cms::util::log::Level::critical);
    clockState.current = 20;
    CMS_TEST_CHECK(logger.log(invalid, "invalid") == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 5);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 4, "[20] [UNKNOWN] invalid\n");

    logger.setMinLevel(cms::util::log::Level::warning);
    logger.setLoggingEnabled(false);
    logger.setLoggingEnabled(true);
    clockState.current = 21;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "enabled-again")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 5, "[21] [WARNING] enabled-again\n");
    CMS_TEST_CHECK(clockState.calls == 6);
    CMS_TEST_CHECK(!sinkState.observedLockedMutex);
    CMS_TEST_CHECK(!sinkState.captureFailed);
    CMS_TEST_CHECK(!queueMutex.locked);
    CMS_TEST_CHECK(queueMutex.locks == queueMutex.unlocks);

    using NullRuntimeLevelLogger = cms::util::log::AsyncLogger<
        16,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::PlainFormatter,
        cms::util::log::RuntimeLevelFilter>;
    ClockState nullClockState;
    SinkState nullSinkState;
    NullRuntimeLevelLogger nullLogger{
        CountingClock(nullClockState),
        CapturingSink(nullSinkState)};
    nullLogger.setMinLevel(cms::util::log::Level::error);
    CMS_TEST_CHECK(nullLogger.log(cms::util::log::Level::info, "hidden")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(nullClockState.calls == 0);
    nullClockState.current = 30;
    CMS_TEST_CHECK(nullLogger.log(cms::util::log::Level::error, "shown")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(nullLogger.drainOne() == cms::util::Status::ok);
    checkLine(nullSinkState, 0, "[30] [ERROR] shown\n");

    using AnsiRuntimeLevelLogger = cms::util::log::AsyncLogger<
        16,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;
    ClockState ansiClockState;
    SinkState ansiSinkState;
    AnsiRuntimeLevelLogger ansiLogger{
        CountingClock(ansiClockState),
        CapturingSink(ansiSinkState)};
    ansiLogger.setMinLevel(cms::util::log::Level::warning);
    CMS_TEST_CHECK(ansiLogger.log(cms::util::log::Level::debug, "hidden")
        == cms::util::Status::ok);
    ansiClockState.current = 40;
    CMS_TEST_CHECK(ansiLogger.log(cms::util::log::Level::warning, "shown")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(ansiLogger.drainOne() == cms::util::Status::ok);
    checkLine(
        ansiSinkState,
        0,
        "[40] \033[33m[WARNING]\033[0m shown\n");

    using MeasuredPlainLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex>;
    using MeasuredAnsiLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter>;
    using MeasuredRuntimeAnsiLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::RuntimeAnsiFormatter>;
    using MeasuredRuntimeLevelPlainLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::PlainFormatter,
        cms::util::log::RuntimeLevelFilter>;
    using MeasuredRuntimeLevelAnsiLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;
    using MeasuredRuntimeBothLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::RuntimeAnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;
    using MeasuredRecord = cms::util::log::StaticRecord<64>;
    using MeasuredQueue = cms::util::StaticQueue<MeasuredRecord, 8>;

    std::printf("sizeof(existing plain logger)=%zu\n",
        sizeof(MeasuredPlainLogger));
    std::printf("sizeof(existing ANSI logger)=%zu\n",
        sizeof(MeasuredAnsiLogger));
    std::printf("sizeof(runtime ANSI logger)=%zu\n",
        sizeof(MeasuredRuntimeAnsiLogger));
    std::printf("sizeof(runtime-level plain logger)=%zu\n",
        sizeof(MeasuredRuntimeLevelPlainLogger));
    std::printf("sizeof(runtime-level ANSI logger)=%zu\n",
        sizeof(MeasuredRuntimeLevelAnsiLogger));
    std::printf("sizeof(runtime-color + runtime-level logger)=%zu\n",
        sizeof(MeasuredRuntimeBothLogger));
    std::printf("sizeof(cms::util::log::StaticRecord<64>)=%zu\n",
        sizeof(MeasuredRecord));
    std::printf("sizeof(cms::util::StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(MeasuredQueue));

    return cms::test::finish();
}
