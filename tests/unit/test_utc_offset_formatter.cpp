#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/level_filter.h>
#include <cms/log/printf_log.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/log/utc_offset_formatter.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/sync/null_mutex.h>

#include "test.h"

namespace {

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

void checkTimestamp(
    cms::log::Timestamp timestamp,
    int offsetMinutes,
    cms::StringView expected) {
    cms::StaticString<9> output;
    const cms::WriteResult result = cms::log::formatUtcOffsetTimestamp(
        timestamp,
        offsetMinutes,
        output.buffer());
    CMS_TEST_REQUIRE(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.written == 8);
    CMS_TEST_CHECK(result.required == 8);
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

template<class Formatter>
void checkFormatted(
    const Formatter& formatter,
    cms::log::Level level,
    cms::log::Timestamp timestamp,
    cms::StringView message,
    cms::StringView expected) {
    cms::StaticString<192> output;
    const cms::WriteResult result = formatter.format(
        {level, timestamp, message},
        output.buffer());
    CMS_TEST_REQUIRE(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.written == expected.size());
    CMS_TEST_CHECK(result.required == expected.size());
    checkBytes(output.view(), expected);
}

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
    cms::StaticString<256> lines[8];
    std::size_t writes = 0;
    bool failed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::StringView value) noexcept {
        if (state_->writes >= 8
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

void checkLine(
    const SinkState& state,
    std::size_t index,
    cms::StringView expected) {
    CMS_TEST_REQUIRE(index < state.writes);
    checkBytes(state.lines[index].view(), expected);
}

} // namespace

int main() {
    checkTimestamp(0, 0, "00:00:00");
    checkTimestamp(999, 0, "00:00:00");
    checkTimestamp(1000, 0, "00:00:01");
    checkTimestamp(0, 540, "09:00:00");
    checkTimestamp(0, 330, "05:30:00");
    checkTimestamp(0, 345, "05:45:00");
    checkTimestamp(0, -240, "20:00:00");
    checkTimestamp(0, -300, "19:00:00");
    checkTimestamp(0, cms::log::minUtcOffsetMinutes, "12:00:00");
    checkTimestamp(0, cms::log::maxUtcOffsetMinutes, "14:00:00");
    checkTimestamp(UINT64_C(53999000), 540, "23:59:59");
    checkTimestamp(UINT64_C(54000000), 540, "00:00:00");
    checkTimestamp(
        (std::numeric_limits<cms::log::Timestamp>::max)(),
        0,
        "14:25:51");

    cms::StringBuffer unbound;
    const cms::WriteResult invalidOutput =
        cms::log::formatUtcOffsetTimestamp(0, 0, unbound);
    CMS_TEST_CHECK(invalidOutput.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(invalidOutput.written == 0);
    CMS_TEST_CHECK(invalidOutput.required == 0);

    cms::StaticString<9> unchanged;
    CMS_TEST_REQUIRE(unchanged.assign("seed").status == cms::Status::ok);
    const cms::WriteResult invalidLower =
        cms::log::formatUtcOffsetTimestamp(0, -721, unchanged.buffer());
    CMS_TEST_CHECK(invalidLower.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(invalidLower.written == 0);
    CMS_TEST_CHECK(invalidLower.required == 0);
    checkBytes(unchanged.view(), "seed");
    const cms::WriteResult invalidUpper =
        cms::log::formatUtcOffsetTimestamp(0, 841, unchanged.buffer());
    CMS_TEST_CHECK(invalidUpper.status == cms::Status::invalid_argument);
    checkBytes(unchanged.view(), "seed");

    cms::StaticString<8> shortOutput;
    CMS_TEST_REQUIRE(shortOutput.assign("seed").status == cms::Status::ok);
    const cms::WriteResult noSpace =
        cms::log::formatUtcOffsetTimestamp(0, 0, shortOutput.buffer());
    CMS_TEST_CHECK(noSpace.status == cms::Status::no_space);
    CMS_TEST_CHECK(noSpace.written == 0);
    CMS_TEST_CHECK(noSpace.required == 8);
    checkBytes(shortOutput.view(), "seed");
    CMS_TEST_CHECK(shortOutput.cStr()[shortOutput.size()] == '\0');

    cms::log::UtcOffsetFormatter<> plain;
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == 0);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "default",
        "[00:00:00] [INFO] default\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(540) == cms::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == 540);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "kst",
        "[09:00:00] [INFO] kst\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(330) == cms::Status::ok);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "india",
        "[05:30:00] [INFO] india\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(345) == cms::Status::ok);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "nepal",
        "[05:45:00] [INFO] nepal\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-240) == cms::Status::ok);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "summer",
        "[20:00:00] [INFO] summer\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-300) == cms::Status::ok);
    checkFormatted(
        plain,
        cms::log::Level::info,
        0,
        "winter",
        "[19:00:00] [INFO] winter\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-721)
        == cms::Status::invalid_argument);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == -300);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(841)
        == cms::Status::invalid_argument);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == -300);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(
        cms::log::minUtcOffsetMinutes) == cms::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes()
        == cms::log::minUtcOffsetMinutes);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(
        cms::log::maxUtcOffsetMinutes) == cms::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes()
        == cms::log::maxUtcOffsetMinutes);

    cms::log::StaticRecord<16> record;
    CMS_TEST_REQUIRE(record.assign(
        cms::log::Level::info,
        UINT64_C(54000000),
        "record").status == cms::Status::ok);
    const cms::log::Timestamp absoluteTimestamp =
        record.timestampMilliseconds();
    cms::StaticString<64> recordOutput;
    CMS_TEST_REQUIRE(plain.format(
        record.view(),
        recordOutput.buffer()).status == cms::Status::ok);
    CMS_TEST_CHECK(record.timestampMilliseconds() == absoluteTimestamp);

    cms::log::UtcOffsetFormatter<cms::log::AnsiFormatter> ansi;
    CMS_TEST_CHECK(ansi.setUtcOffsetMinutes(540) == cms::Status::ok);
    checkFormatted(
        ansi,
        cms::log::Level::error,
        UINT64_C(54000000),
        "ansi",
        "[00:00:00] \033[31m[ERROR]\033[0m ansi\n");

    cms::log::UtcOffsetFormatter<cms::log::RuntimeAnsiFormatter>
        runtimeAnsi;
    CMS_TEST_CHECK(runtimeAnsi.setUtcOffsetMinutes(330) == cms::Status::ok);
    checkFormatted(
        runtimeAnsi,
        cms::log::Level::warning,
        0,
        "color",
        "[05:30:00] \033[33m[WARNING]\033[0m color\n");
    runtimeAnsi.setUseColor(false);
    checkFormatted(
        runtimeAnsi,
        cms::log::Level::warning,
        0,
        "plain",
        "[05:30:00] [WARNING] plain\n");

    cms::log::UtcOffsetFormatter<cms::log::StyledAnsiFormatter> styled;
    CMS_TEST_CHECK(styled.setUtcOffsetMinutes(345) == cms::Status::ok);
    checkFormatted(
        styled,
        cms::log::Level::error,
        0,
        "[NET] FAIL",
        "[05:45:00] \033[31m[ERROR]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mFAIL\033[0m\n");

    using RuntimeFormatter =
        cms::log::UtcOffsetFormatter<
            cms::log::RuntimeStyledAnsiFormatter>;
    using Logger = cms::log::AsyncLogger<
        64,
        4,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        RuntimeFormatter,
        cms::log::RuntimeLevelFilter>;

    ClockState clockState;
    SinkState sinkState;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};
    CMS_TEST_CHECK(logger.utcOffsetMinutes() == 0);
    logger.setMinLevel(cms::log::Level::warning);
    CMS_TEST_CHECK(logger.log(cms::log::Level::debug, "filtered")
        == cms::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);
    logger.setLoggingEnabled(false);
    CMS_TEST_CHECK(logger.log(cms::log::Level::critical, "disabled")
        == cms::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);

    logger.setLoggingEnabled(true);
    logger.setUseColor(true);
    clockState.current = 0;
    CMS_TEST_CHECK(cms::log::logf(
        logger,
        cms::log::Level::warning,
        "[NET] value=%d",
        7) == cms::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(540) == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        0,
        "[09:00:00] \033[33m[WARNING]\033[0m \033[95m[NET]\033[0m "
        "value=7\n");

    clockState.current = 0;
    CMS_TEST_CHECK(logger.log(cms::log::Level::error, "queued")
        == cms::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-240) == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        1,
        "[20:00:00] \033[31m[ERROR]\033[0m queued\n");
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-300) == cms::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-721)
        == cms::Status::invalid_argument);
    CMS_TEST_CHECK(logger.utcOffsetMinutes() == -300);
    logger.setUseColor(false);
    CMS_TEST_CHECK(cms::log::logf(
        logger,
        cms::log::Level::critical,
        "value=%d",
        8) == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    checkLine(
        sinkState,
        2,
        "[19:00:00] [CRITICAL] value=8\n");
    CMS_TEST_CHECK(clockState.calls == 3);
    CMS_TEST_CHECK(!sinkState.failed);

    using UtcPlainLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::UtcOffsetFormatter<>>;
    using UtcRuntimeAnsiLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        cms::log::UtcOffsetFormatter<cms::log::RuntimeAnsiFormatter>>;
    using UtcRuntimeStyledLevelLogger = cms::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::sync::NullMutex,
        RuntimeFormatter,
        cms::log::RuntimeLevelFilter>;
    using QueueRecord = cms::log::StaticRecord<64>;
    using Queue = cms::StaticQueue<QueueRecord, 8>;

    std::printf("sizeof(UTC offset plain logger)=%zu\n",
        sizeof(UtcPlainLogger));
    std::printf("sizeof(UTC offset runtime ANSI logger)=%zu\n",
        sizeof(UtcRuntimeAnsiLogger));
    std::printf("sizeof(UTC offset runtime styled + level logger)=%zu\n",
        sizeof(UtcRuntimeStyledLevelLogger));
    std::printf("sizeof(StaticRecord<64>)=%zu\n", sizeof(QueueRecord));
    std::printf("sizeof(StaticQueue<StaticRecord<64>, 8>)=%zu\n",
        sizeof(Queue));
    std::printf("UTC offset formatter local timestamp storage=%zu bytes\n",
        sizeof(cms::StaticString<cms::log::utcOffsetTimestampSize + 1>));

    return cms::test::finish();
}
