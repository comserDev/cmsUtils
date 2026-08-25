#include <cstddef>
#include <cstdio>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>

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
    cms::log::Timestamp current = 0;
    std::size_t calls = 0;
};

struct CountingClock {
    explicit CountingClock(ClockState& state) noexcept
        : state_(&state) {}

    cms::log::Timestamp nowMilliseconds() noexcept {
        ++state_->calls;
        return state_->current;
    }

private:
    ClockState* state_;
};

struct SinkState {
    cms::StaticString<128> lines[16];
    std::size_t writes = 0;
    CountingMutex* queueMutex = nullptr;
    bool observedLockedMutex = false;
    bool captureFailed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept
        : state_(&state) {}

    void write(cms::StringView text) noexcept {
        if (state_->queueMutex != nullptr && state_->queueMutex->locked) {
            state_->observedLockedMutex = true;
        }
        if (state_->writes >= 16) {
            state_->captureFailed = true;
            return;
        }
        if (state_->lines[state_->writes].assign(text).status
            != cms::Status::ok) {
            state_->captureFailed = true;
        }
        ++state_->writes;
    }

private:
    SinkState* state_;
};

void checkBytes(cms::StringView actual, cms::StringView expected) {
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
    cms::StringView expected) {
    CMS_TEST_REQUIRE(index < state.writes);
    checkBytes(state.lines[index].view(), expected);
}

} // namespace

int main() {
    using RuntimeLogger = cms::log::AsyncLogger<
        16,
        3,
        CountingClock,
        CapturingSink,
        cms::sync::MutexRef<CountingMutex>,
        cms::log::RuntimeAnsiFormatter>;

    ClockState clockState;
    CountingMutex queueMutex;
    SinkState sinkState;
    sinkState.queueMutex = &queueMutex;
    RuntimeLogger logger{
        CountingClock(clockState),
        CapturingSink(sinkState),
        cms::sync::MutexRef<CountingMutex>(queueMutex)};

    CMS_TEST_CHECK(logger.useColor());
    CMS_TEST_CHECK(logger.capacity() == 3);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::out_of_range);
    CMS_TEST_CHECK(sinkState.writes == 0);

    const int locksBeforeSwitch = queueMutex.locks;
    const int unlocksBeforeSwitch = queueMutex.unlocks;
    logger.setUseColor(false);
    CMS_TEST_CHECK(!logger.useColor());
    CMS_TEST_CHECK(queueMutex.locks == locksBeforeSwitch);
    CMS_TEST_CHECK(queueMutex.unlocks == unlocksBeforeSwitch);

    char first[] = "one";
    clockState.current = 10;
    CMS_TEST_CHECK(logger.log(cms::log::Level::info, first)
        == cms::Status::ok);
    first[0] = 'X';
    clockState.current = 999;
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(sinkState, 0, "[10] [INFO] one\n");
    CMS_TEST_CHECK(clockState.calls == 1);

    logger.setUseColor(true);
    CMS_TEST_CHECK(logger.useColor());
    clockState.current = 20;
    CMS_TEST_CHECK(logger.log(cms::log::Level::info, "two")
        == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        1,
        "[20] \033[32m[INFO]\033[0m two\n");

    logger.setUseColor(false);
    CMS_TEST_CHECK(!logger.useColor());
    clockState.current = 30;
    CMS_TEST_CHECK(logger.log(cms::log::Level::info, "three")
        == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(sinkState, 2, "[30] [INFO] three\n");

    char queued[] = "queued";
    clockState.current = 40;
    CMS_TEST_CHECK(logger.log(cms::log::Level::warning, queued)
        == cms::Status::ok);
    queued[0] = 'X';
    logger.setUseColor(true);
    clockState.current = 400;
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        3,
        "[40] \033[33m[WARNING]\033[0m queued\n");
    CMS_TEST_CHECK(clockState.calls == 4);

    logger.setUseColor(false);
    clockState.current = 50;
    CMS_TEST_CHECK(logger.log(cms::log::Level::debug, "a")
        == cms::Status::ok);
    clockState.current = 51;
    CMS_TEST_CHECK(logger.log(cms::log::Level::info, "b")
        == cms::Status::ok);
    clockState.current = 52;
    CMS_TEST_CHECK(logger.log(cms::log::Level::error, "c")
        == cms::Status::ok);
    CMS_TEST_CHECK(logger.full());
    clockState.current = 53;
    CMS_TEST_CHECK(logger.log(cms::log::Level::critical, "drop")
        == cms::Status::no_space);
    CMS_TEST_CHECK(logger.pending() == 3);

    logger.setUseColor(true);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        4,
        "[50] \033[36m[DEBUG]\033[0m a\n");
    checkLine(
        sinkState,
        5,
        "[51] \033[32m[INFO]\033[0m b\n");
    checkLine(
        sinkState,
        6,
        "[52] \033[31m[ERROR]\033[0m c\n");
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(clockState.calls == 8);
    CMS_TEST_CHECK(!sinkState.observedLockedMutex);
    CMS_TEST_CHECK(!sinkState.captureFailed);
    CMS_TEST_CHECK(!queueMutex.locked);
    CMS_TEST_CHECK(queueMutex.locks == queueMutex.unlocks);

    using NullRuntimeLogger = cms::log::AsyncLogger<
        16,
        1,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::RuntimeAnsiFormatter>;
    ClockState nullClockState;
    nullClockState.current = 77;
    SinkState nullSinkState;
    NullRuntimeLogger nullLogger{
        CountingClock(nullClockState),
        CapturingSink(nullSinkState)};
    CMS_TEST_CHECK(nullLogger.useColor());
    nullLogger.setUseColor(false);
    CMS_TEST_CHECK(nullLogger.log(cms::log::Level::critical, "null")
        == cms::Status::ok);
    CMS_TEST_CHECK(nullLogger.drainOne() == cms::Status::ok);
    checkLine(nullSinkState, 0, "[77] [CRITICAL] null\n");

    using MeasuredPlainLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex>;
    using MeasuredAnsiLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::AnsiFormatter>;
    using MeasuredRuntimeLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::RuntimeAnsiFormatter>;
    using MeasuredRecord = cms::log::StaticRecord<64>;
    using MeasuredQueue = cms::StaticQueue<MeasuredRecord, 8>;

    std::printf(
        "sizeof(plain AsyncLogger<64, 8, ...>)=%zu\n",
        sizeof(MeasuredPlainLogger));
    std::printf(
        "sizeof(ANSI AsyncLogger<64, 8, ...>)=%zu\n",
        sizeof(MeasuredAnsiLogger));
    std::printf(
        "sizeof(runtime ANSI AsyncLogger<64, 8, ...>)=%zu\n",
        sizeof(MeasuredRuntimeLogger));
    std::printf(
        "sizeof(cms::log::StaticRecord<64>)=%zu\n",
        sizeof(MeasuredRecord));
    std::printf(
        "sizeof(cms::StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(MeasuredQueue));

    return cms::test::finish();
}
