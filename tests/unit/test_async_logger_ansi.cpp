#include <cstddef>
#include <cstdio>
#include <limits>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/async_logger.h>
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
    cms::util::StaticString<128> lines[8];
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
        if (state_->writes >= 8) {
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
    using AnsiLogger = cms::util::log::AsyncLogger<
        16,
        2,
        CountingClock,
        CapturingSink,
        cms::util::sync::MutexRef<CountingMutex>,
        cms::util::log::AnsiFormatter>;

    ClockState clockState;
    CountingMutex queueMutex;
    SinkState sinkState;
    sinkState.queueMutex = &queueMutex;
    AnsiLogger logger{
        CountingClock(clockState),
        CapturingSink(sinkState),
        cms::util::sync::MutexRef<CountingMutex>(queueMutex)};

    CMS_TEST_CHECK(logger.capacity() == 2);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);
    CMS_TEST_CHECK(sinkState.writes == 0);

    char source[] = "hello";
    clockState.current = 10;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, source)
        == cms::util::Status::ok);
    clockState.current = 20;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "one")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.full());
    clockState.current = 30;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "drop")
        == cms::util::Status::no_space);
    CMS_TEST_CHECK(clockState.calls == 3);
    source[0] = 'X';
    clockState.current = 999;

    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        0,
        "[10] \033[32m[INFO]\033[0m hello\n");
    CMS_TEST_CHECK(clockState.calls == 3);

    clockState.current = 21;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "two")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.full());
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        1,
        "[20] \033[36m[DEBUG]\033[0m one\n");
    checkLine(
        sinkState,
        2,
        "[21] \033[33m[WARNING]\033[0m two\n");
    CMS_TEST_CHECK(clockState.calls == 4);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(!sinkState.observedLockedMutex);
    CMS_TEST_CHECK(!sinkState.captureFailed);
    CMS_TEST_CHECK(!queueMutex.locked);
    CMS_TEST_CHECK(queueMutex.locks == queueMutex.unlocks);

    using NullAnsiLogger = cms::util::log::AsyncLogger<
        16,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter>;
    ClockState nullClockState;
    nullClockState.current = 77;
    SinkState nullSinkState;
    NullAnsiLogger nullLogger{
        CountingClock(nullClockState),
        CapturingSink(nullSinkState)};
    CMS_TEST_CHECK(nullLogger.log(cms::util::log::Level::error, "null")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(nullLogger.drainOne() == cms::util::Status::ok);
    checkLine(
        nullSinkState,
        0,
        "[77] \033[31m[ERROR]\033[0m null\n");

    using PlainLogger = cms::util::log::AsyncLogger<
        16,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;
    ClockState plainClockState;
    plainClockState.current = 5;
    SinkState plainSinkState;
    PlainLogger plainLogger{
        CountingClock(plainClockState),
        CapturingSink(plainSinkState)};
    CMS_TEST_CHECK(plainLogger.log(cms::util::log::Level::info, "plain")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(plainLogger.drainOne() == cms::util::Status::ok);
    checkLine(plainSinkState, 0, "[5] [INFO] plain\n");

    using MaximumAnsiLogger = cms::util::log::AsyncLogger<
        64,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter>;
    ClockState maximumClockState;
    maximumClockState.current =
        (std::numeric_limits<cms::util::log::Timestamp>::max)();
    SinkState maximumSinkState;
    MaximumAnsiLogger maximumLogger{
        CountingClock(maximumClockState),
        CapturingSink(maximumSinkState)};
    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); ++index) {
        maximumMessage[index] = 'm';
    }
    CMS_TEST_CHECK(maximumLogger.log(
        cms::util::log::Level::warning,
        cms::util::StringView(maximumMessage, sizeof(maximumMessage)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumLogger.drainOne() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumSinkState.writes == 1);
    cms::util::StaticString<64 + cms::util::log::maxAnsiFormattedRecordOverhead>
        maximumExpected;
    CMS_TEST_REQUIRE(maximumExpected.assign(
        "[18446744073709551615] \033[33m[WARNING]\033[0m ").status
        == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumExpected.append(
        cms::util::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumExpected.append("\n").status == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumExpected.size() == 106);
    checkLine(maximumSinkState, 0, maximumExpected.view());

    using MeasuredPlainLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;
    using MeasuredAnsiLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter>;
    std::printf(
        "sizeof(plain AsyncLogger<64, 8, ...>)=%zu\n",
        sizeof(MeasuredPlainLogger));
    std::printf(
        "sizeof(ANSI AsyncLogger<64, 8, ...>)=%zu\n",
        sizeof(MeasuredAnsiLogger));

    return cms::test::finish();
}
