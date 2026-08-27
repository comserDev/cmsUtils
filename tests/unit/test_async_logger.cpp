#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

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

struct DefaultClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct DiscardSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
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
    using ExternalLogger = cms::util::log::AsyncLogger<
        16,
        3,
        CountingClock,
        CapturingSink,
        cms::util::sync::MutexRef<CountingMutex>>;

    ClockState clockState;
    CountingMutex queueMutex;
    SinkState sinkState;
    sinkState.queueMutex = &queueMutex;
    ExternalLogger logger{
        CountingClock(clockState),
        CapturingSink(sinkState),
        cms::util::sync::MutexRef<CountingMutex>(queueMutex)};

    CMS_TEST_CHECK(logger.capacity() == 3);
    CMS_TEST_CHECK(logger.pending() == 0);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(!logger.full());

    const std::size_t writesBeforeEmptyDrain = sinkState.writes;
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);
    CMS_TEST_CHECK(sinkState.writes == writesBeforeEmptyDrain);

    char tooLong[16] = {};
    for (std::size_t index = 0; index < sizeof(tooLong); ++index) {
        tooLong[index] = 'x';
    }
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::info,
        cms::util::StringView(tooLong, sizeof(tooLong))) == cms::util::Status::no_space);
    CMS_TEST_CHECK(clockState.calls == 0);
    CMS_TEST_CHECK(logger.pending() == 0);

    char source[] = "hello";
    clockState.current = 10;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::info,
        cms::util::StringView(source)) == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 1);
    source[0] = 'X';
    clockState.current = 999;
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 1);
    CMS_TEST_CHECK(sinkState.writes == 1);
    checkLine(sinkState, 0, "[10] [INFO] hello\n");
    CMS_TEST_CHECK(logger.empty());

    clockState.current = 20;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "one") == cms::util::Status::ok);
    clockState.current = 21;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "two") == cms::util::Status::ok);
    clockState.current = 22;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::warning, "three") == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.pending() == 3);
    CMS_TEST_CHECK(logger.full());

    clockState.current = 23;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::critical, "dropped") == cms::util::Status::no_space);
    CMS_TEST_CHECK(clockState.calls == 5);
    CMS_TEST_CHECK(logger.pending() == 3);

    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 1, "[20] [INFO] one\n");
    CMS_TEST_CHECK(logger.pending() == 2);

    clockState.current = 24;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::error, "four") == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.pending() == 3);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 2, "[21] [DEBUG] two\n");
    checkLine(sinkState, 3, "[22] [WARNING] three\n");
    checkLine(sinkState, 4, "[24] [ERROR] four\n");
    CMS_TEST_CHECK(sinkState.writes == 5);
    CMS_TEST_CHECK(clockState.calls == 6);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(logger.pending() == 0);
    CMS_TEST_CHECK(!sinkState.observedLockedMutex);
    CMS_TEST_CHECK(!sinkState.captureFailed);
    CMS_TEST_CHECK(!queueMutex.locked);
    CMS_TEST_CHECK(queueMutex.locks == queueMutex.unlocks);

    const std::size_t writesBeforeFinalEmptyDrain = sinkState.writes;
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);
    CMS_TEST_CHECK(sinkState.writes == writesBeforeFinalEmptyDrain);

    using NullLogger = cms::util::log::AsyncLogger<
        16,
        2,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;

    ClockState nullClockState;
    nullClockState.current = 77;
    SinkState nullSinkState;
    NullLogger nullLogger{
        CountingClock(nullClockState),
        CapturingSink(nullSinkState)};
    CMS_TEST_CHECK(nullLogger.log(
        cms::util::log::Level::warning, "null") == cms::util::Status::ok);
    CMS_TEST_CHECK(nullLogger.drainOne() == cms::util::Status::ok);
    checkLine(nullSinkState, 0, "[77] [WARNING] null\n");
    CMS_TEST_CHECK(nullClockState.calls == 1);
    CMS_TEST_CHECK(nullSinkState.writes == 1);
    CMS_TEST_CHECK(nullLogger.empty());

    using DefaultLogger = cms::util::log::AsyncLogger<
        8,
        2,
        DefaultClock,
        DiscardSink,
        cms::util::sync::NullMutex>;
    DefaultLogger defaultLogger;
    CMS_TEST_CHECK(defaultLogger.empty());
    CMS_TEST_CHECK(defaultLogger.capacity() == 2);

    using MaximumLogger = cms::util::log::AsyncLogger<
        64,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;
    ClockState maximumClockState;
    maximumClockState.current =
        (std::numeric_limits<cms::util::log::Timestamp>::max)();
    SinkState maximumSinkState;
    MaximumLogger maximumLogger{
        CountingClock(maximumClockState),
        CapturingSink(maximumSinkState)};
    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); ++index) {
        maximumMessage[index] = 'm';
    }
    CMS_TEST_CHECK(maximumLogger.log(
        cms::util::log::Level::critical,
        cms::util::StringView(maximumMessage, sizeof(maximumMessage)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumLogger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumClockState.calls == 1);
    CMS_TEST_CHECK(maximumSinkState.writes == 1);
    CMS_TEST_CHECK(!maximumSinkState.captureFailed);

    cms::util::StaticString<64 + cms::util::log::maxFormattedRecordOverhead>
        maximumExpected;
    CMS_TEST_REQUIRE(maximumExpected.assign(
        "[18446744073709551615] [CRITICAL] ").status == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumExpected.append(
        cms::util::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumExpected.append("\n").status == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumExpected.size() == 98);
    checkLine(maximumSinkState, 0, maximumExpected.view());

    using MeasuredLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;
    std::printf(
        "sizeof(cms::util::log::AsyncLogger<64, 8, CountingClock, "
        "CapturingSink, NullMutex>)=%zu\n",
        sizeof(MeasuredLogger));
    std::printf(
        "drain local StaticRecord<64> storage=%zu\n",
        sizeof(cms::util::log::StaticRecord<64>));
    std::printf(
        "drain local formatted line storage=%zu\n",
        sizeof(cms::util::StaticString<
            64 + cms::util::log::maxFormattedRecordOverhead>));

    return cms::test::finish();
}
