#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/printf_log.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/styled_ansi_formatter.h>
#include <cms/util/log/utc_offset_formatter.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

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

void checkTimestamp(
    cms::util::log::Timestamp timestamp,
    int offsetMinutes,
    cms::util::StringView expected) {
    cms::util::StaticString<9> output;
    const cms::util::WriteResult result = cms::util::log::formatUtcOffsetTimestamp(
        timestamp,
        offsetMinutes,
        output.buffer());
    CMS_TEST_REQUIRE(result.status == cms::util::Status::ok);
    CMS_TEST_CHECK(result.written == 8);
    CMS_TEST_CHECK(result.required == 8);
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

template<class Formatter>
void checkFormatted(
    const Formatter& formatter,
    cms::util::log::Level level,
    cms::util::log::Timestamp timestamp,
    cms::util::StringView message,
    cms::util::StringView expected) {
    cms::util::StaticString<192> output;
    const cms::util::WriteResult result = formatter.format(
        {level, timestamp, message},
        output.buffer());
    CMS_TEST_REQUIRE(result.status == cms::util::Status::ok);
    CMS_TEST_CHECK(result.written == expected.size());
    CMS_TEST_CHECK(result.required == expected.size());
    checkBytes(output.view(), expected);
}

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
    cms::util::StaticString<256> lines[8];
    std::size_t writes = 0;
    bool failed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::util::StringView value) noexcept {
        if (state_->writes >= 8
            || state_->lines[state_->writes].assign(value).status
                != cms::util::Status::ok) {
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
    cms::util::StringView expected) {
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
    checkTimestamp(0, cms::util::log::minUtcOffsetMinutes, "12:00:00");
    checkTimestamp(0, cms::util::log::maxUtcOffsetMinutes, "14:00:00");
    checkTimestamp(UINT64_C(53999000), 540, "23:59:59");
    checkTimestamp(UINT64_C(54000000), 540, "00:00:00");
    checkTimestamp(
        (std::numeric_limits<cms::util::log::Timestamp>::max)(),
        0,
        "14:25:51");

    cms::util::StringBuffer unbound;
    const cms::util::WriteResult invalidOutput =
        cms::util::log::formatUtcOffsetTimestamp(0, 0, unbound);
    CMS_TEST_CHECK(invalidOutput.status == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(invalidOutput.written == 0);
    CMS_TEST_CHECK(invalidOutput.required == 0);

    cms::util::StaticString<9> unchanged;
    CMS_TEST_REQUIRE(unchanged.assign("seed").status == cms::util::Status::ok);
    const cms::util::WriteResult invalidLower =
        cms::util::log::formatUtcOffsetTimestamp(0, -721, unchanged.buffer());
    CMS_TEST_CHECK(invalidLower.status == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(invalidLower.written == 0);
    CMS_TEST_CHECK(invalidLower.required == 0);
    checkBytes(unchanged.view(), "seed");
    const cms::util::WriteResult invalidUpper =
        cms::util::log::formatUtcOffsetTimestamp(0, 841, unchanged.buffer());
    CMS_TEST_CHECK(invalidUpper.status == cms::util::Status::invalid_argument);
    checkBytes(unchanged.view(), "seed");

    cms::util::StaticString<8> shortOutput;
    CMS_TEST_REQUIRE(shortOutput.assign("seed").status == cms::util::Status::ok);
    const cms::util::WriteResult noSpace =
        cms::util::log::formatUtcOffsetTimestamp(0, 0, shortOutput.buffer());
    CMS_TEST_CHECK(noSpace.status == cms::util::Status::no_space);
    CMS_TEST_CHECK(noSpace.written == 0);
    CMS_TEST_CHECK(noSpace.required == 8);
    checkBytes(shortOutput.view(), "seed");
    CMS_TEST_CHECK(shortOutput.cStr()[shortOutput.size()] == '\0');

    cms::util::log::UtcOffsetFormatter<> plain;
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == 0);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "default",
        "[00:00:00] [INFO] default\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(540) == cms::util::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == 540);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "kst",
        "[09:00:00] [INFO] kst\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(330) == cms::util::Status::ok);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "india",
        "[05:30:00] [INFO] india\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(345) == cms::util::Status::ok);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "nepal",
        "[05:45:00] [INFO] nepal\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-240) == cms::util::Status::ok);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "summer",
        "[20:00:00] [INFO] summer\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-300) == cms::util::Status::ok);
    checkFormatted(
        plain,
        cms::util::log::Level::info,
        0,
        "winter",
        "[19:00:00] [INFO] winter\n");
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(-721)
        == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == -300);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(841)
        == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(plain.utcOffsetMinutes() == -300);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(
        cms::util::log::minUtcOffsetMinutes) == cms::util::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes()
        == cms::util::log::minUtcOffsetMinutes);
    CMS_TEST_CHECK(plain.setUtcOffsetMinutes(
        cms::util::log::maxUtcOffsetMinutes) == cms::util::Status::ok);
    CMS_TEST_CHECK(plain.utcOffsetMinutes()
        == cms::util::log::maxUtcOffsetMinutes);

    cms::util::log::StaticRecord<16> record;
    CMS_TEST_REQUIRE(record.assign(
        cms::util::log::Level::info,
        UINT64_C(54000000),
        "record").status == cms::util::Status::ok);
    const cms::util::log::Timestamp absoluteTimestamp =
        record.timestampMilliseconds();
    cms::util::StaticString<64> recordOutput;
    CMS_TEST_REQUIRE(plain.format(
        record.view(),
        recordOutput.buffer()).status == cms::util::Status::ok);
    CMS_TEST_CHECK(record.timestampMilliseconds() == absoluteTimestamp);

    cms::util::log::UtcOffsetFormatter<cms::util::log::AnsiFormatter> ansi;
    CMS_TEST_CHECK(ansi.setUtcOffsetMinutes(540) == cms::util::Status::ok);
    checkFormatted(
        ansi,
        cms::util::log::Level::error,
        UINT64_C(54000000),
        "ansi",
        "[00:00:00] \033[31m[ERROR]\033[0m ansi\n");

    cms::util::log::UtcOffsetFormatter<cms::util::log::RuntimeAnsiFormatter>
        runtimeAnsi;
    CMS_TEST_CHECK(runtimeAnsi.setUtcOffsetMinutes(330) == cms::util::Status::ok);
    checkFormatted(
        runtimeAnsi,
        cms::util::log::Level::warning,
        0,
        "color",
        "[05:30:00] \033[33m[WARNING]\033[0m color\n");
    runtimeAnsi.setUseColor(false);
    checkFormatted(
        runtimeAnsi,
        cms::util::log::Level::warning,
        0,
        "plain",
        "[05:30:00] [WARNING] plain\n");

    cms::util::log::UtcOffsetFormatter<cms::util::log::StyledAnsiFormatter> styled;
    CMS_TEST_CHECK(styled.setUtcOffsetMinutes(345) == cms::util::Status::ok);
    checkFormatted(
        styled,
        cms::util::log::Level::error,
        0,
        "[NET] FAIL",
        "[05:45:00] \033[31m[ERROR]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mFAIL\033[0m\n");

    using RuntimeFormatter =
        cms::util::log::UtcOffsetFormatter<
            cms::util::log::RuntimeStyledAnsiFormatter>;
    using Logger = cms::util::log::AsyncLogger<
        64,
        4,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        RuntimeFormatter,
        cms::util::log::RuntimeLevelFilter>;

    ClockState clockState;
    SinkState sinkState;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};
    CMS_TEST_CHECK(logger.utcOffsetMinutes() == 0);
    logger.setMinLevel(cms::util::log::Level::warning);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "filtered")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);
    logger.setLoggingEnabled(false);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "disabled")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);

    logger.setLoggingEnabled(true);
    logger.setUseColor(true);
    clockState.current = 0;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::warning,
        "[NET] value=%d",
        7) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(540) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        0,
        "[09:00:00] \033[33m[WARNING]\033[0m \033[95m[NET]\033[0m "
        "value=7\n");

    clockState.current = 0;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "queued")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-240) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        1,
        "[20:00:00] \033[31m[ERROR]\033[0m queued\n");
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-300) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.setUtcOffsetMinutes(-721)
        == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(logger.utcOffsetMinutes() == -300);
    logger.setUseColor(false);
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::critical,
        "value=%d",
        8) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        2,
        "[19:00:00] [CRITICAL] value=8\n");
    CMS_TEST_CHECK(clockState.calls == 3);
    CMS_TEST_CHECK(!sinkState.failed);

    using UtcPlainLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::UtcOffsetFormatter<>>;
    using UtcRuntimeAnsiLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::UtcOffsetFormatter<cms::util::log::RuntimeAnsiFormatter>>;
    using UtcRuntimeStyledLevelLogger = cms::util::log::AsyncLogger<
        64,
        8,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        RuntimeFormatter,
        cms::util::log::RuntimeLevelFilter>;
    using QueueRecord = cms::util::log::StaticRecord<64>;
    using Queue = cms::util::StaticQueue<QueueRecord, 8>;

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
        sizeof(cms::util::StaticString<cms::util::log::utcOffsetTimestampSize + 1>));

    return cms::test::finish();
}
