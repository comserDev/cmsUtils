#include <cstddef>
#include <cstdio>
#include <limits>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/styled_ansi_formatter.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

struct ClockState {
    cms::util::log::Timestamp current = 0;
    std::size_t calls = 0;
};

struct CountingClock {
    explicit CountingClock(ClockState& state) noexcept : state_(&state) {}

    cms::util::log::Timestamp nowMilliseconds() noexcept {
        ++state_->calls;
        return state_->current;
    }

private:
    ClockState* state_;
};

struct SinkState {
    cms::util::StaticString<512> lines[8];
    std::size_t writes = 0;
    bool failed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    cms::util::Status write(cms::util::StringView value) noexcept {
        if (state_->writes >= 8
            || state_->lines[state_->writes].assign(value).status
                != cms::util::Status::ok) {
            state_->failed = true;
            return cms::util::Status::io_error;
        }
        ++state_->writes;
        return cms::util::Status::ok;
    }

private:
    SinkState* state_;
};

void checkBytes(cms::util::StringView actual, cms::util::StringView expected) {
    CMS_TEST_REQUIRE(actual.size() == expected.size());
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
    using Logger = cms::util::log::AsyncLogger<
        32,
        4,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::RuntimeStyledAnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;

    ClockState clockState;
    SinkState sinkState;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};

    CMS_TEST_CHECK(logger.useColor());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::debug);

    logger.setUseColor(false);
    clockState.current = 1;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "[A] FAIL")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 0, "[1] [INFO] [A] FAIL\n");

    logger.setUseColor(true);
    clockState.current = 2;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "[A] FAIL")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        1,
        "[2] \033[32m[INFO]\033[0m \033[32m[A]\033[0m "
        "\033[1;91mFAIL\033[0m\n");

    logger.setUseColor(false);
    clockState.current = 3;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "[NET] ERROR")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 2, "[3] [CRITICAL] [NET] ERROR\n");

    logger.setMinLevel(cms::util::log::Level::warning);
    logger.setUseColor(true);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "[A] FAIL")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 3);
    CMS_TEST_CHECK(logger.pending() == 0);

    clockState.current = 4;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "[NET] ERROR")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        3,
        "[4] \033[33m[WARNING]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mERROR\033[0m\n");
    CMS_TEST_CHECK(clockState.calls == 4);
    CMS_TEST_CHECK(!sinkState.failed);

    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); index += 3) {
        maximumMessage[index] = '[';
        maximumMessage[index + 1] = 'x';
        maximumMessage[index + 2] = ']';
    }
    using MaximumLogger = cms::util::log::AsyncLogger<
        64,
        1,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::StyledAnsiFormatter>;
    ClockState maximumClock;
    SinkState maximumSink;
    maximumClock.current =
        (std::numeric_limits<cms::util::log::Timestamp>::max)();
    MaximumLogger maximumLogger{
        CountingClock(maximumClock),
        CapturingSink(maximumSink)};
    CMS_TEST_CHECK(maximumLogger.log(
        cms::util::log::Level::warning,
        cms::util::StringView(maximumMessage, sizeof(maximumMessage)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(maximumLogger.drainOne() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(maximumSink.writes == 1);
    CMS_TEST_CHECK(maximumSink.lines[0].size() == 295);
    CMS_TEST_CHECK(maximumSink.lines[0].view()[294] == '\n');
    CMS_TEST_CHECK(maximumSink.lines[0].cStr()[295] == '\0');
    CMS_TEST_CHECK(!maximumSink.failed);

    using PlainLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex>;
    using AnsiLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::AnsiFormatter>;
    using RuntimeAnsiLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::RuntimeAnsiFormatter>;
    using RuntimeLevelLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::PlainFormatter, cms::util::log::RuntimeLevelFilter>;
    using StyledLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::StyledAnsiFormatter>;
    using RuntimeStyledLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::RuntimeStyledAnsiFormatter>;
    using RuntimeStyledLevelLogger = cms::util::log::AsyncLogger<
        64, 8, CountingClock, CapturingSink, cms::util::sync::NullMutex,
        cms::util::log::RuntimeStyledAnsiFormatter, cms::util::log::RuntimeLevelFilter>;
    using Record = cms::util::log::StaticRecord<64>;
    using Queue = cms::util::StaticQueue<Record, 8>;

    std::printf("sizeof(existing plain logger)=%zu\n", sizeof(PlainLogger));
    std::printf("sizeof(existing ANSI logger)=%zu\n", sizeof(AnsiLogger));
    std::printf("sizeof(existing runtime ANSI logger)=%zu\n",
        sizeof(RuntimeAnsiLogger));
    std::printf("sizeof(existing runtime level logger)=%zu\n",
        sizeof(RuntimeLevelLogger));
    std::printf("sizeof(styled logger)=%zu\n", sizeof(StyledLogger));
    std::printf("sizeof(runtime styled logger)=%zu\n",
        sizeof(RuntimeStyledLogger));
    std::printf("sizeof(runtime styled + level logger)=%zu\n",
        sizeof(RuntimeStyledLevelLogger));
    std::printf("sizeof(StaticRecord<64>)=%zu\n", sizeof(Record));
    std::printf("sizeof(StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(Queue));

    return cms::test::finish();
}
