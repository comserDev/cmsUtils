#include <cstddef>
#include <cstdint>

#include <cms/util/log/printf_log.h>
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
    cms::util::StaticString<256> lines[20];
    std::size_t writes = 0;
    bool failed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::util::StringView value) noexcept {
        if (state_->writes >= 20
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
    using Logger = cms::util::log::AsyncLogger<
        64,
        4,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;

    ClockState clockState;
    SinkState sinkState;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};

    CMS_TEST_CHECK(logger.wouldLog(cms::util::log::Level::trace));
    CMS_TEST_CHECK(logger.wouldLog(static_cast<cms::util::log::Level>(0xFF)));

    clockState.current = 1;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger, cms::util::log::Level::info, "literal") == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 0, "[1] [INFO] literal\n");

    clockState.current = 2;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger, cms::util::log::Level::info, "%d", 42) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 1, "[2] [INFO] 42\n");

    clockState.current = 3;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger, cms::util::log::Level::info, "%d", -42) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 2, "[3] [INFO] -42\n");

    clockState.current = 4;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        "%u %x",
        4000000000U,
        0xBEEFU) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 3, "[4] [INFO] 4000000000 beef\n");

    clockState.current = 5;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        "%s %c %%%%",
        "text",
        'Z') == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 4, "[5] [INFO] text Z %%\n");

    clockState.current = 6;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        "%05d %.3s %.2f",
        42,
        "abcdef",
        85.43) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 5, "[6] [INFO] 00042 abc 85.43\n");

    clockState.current = 7;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::warning,
        "%s=%d/%u/%x",
        "value",
        -7,
        8U,
        9U) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 6, "[7] [WARNING] value=-7/8/9\n");

    clockState.current = 8;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger, cms::util::log::Level::info, "") == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 7, "[8] [INFO] \n");

    clockState.current = 9;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::error,
        "A%cB",
        '\0') == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    const char embeddedExpected[] = {
        '[', '9', ']', ' ', '[', 'E', 'R', 'R', 'O', 'R', ']', ' ',
        'A', '\0', 'B', '\n'};
    checkLine(
        sinkState,
        8,
        cms::util::StringView(embeddedExpected, sizeof(embeddedExpected)));

    const char utf8Format[] = {
        static_cast<char>(0xEA), static_cast<char>(0xB0),
        static_cast<char>(0x80), '=', '%', 'd', '\0'};
    const char utf8Expected[] = {
        '[', '1', '0', ']', ' ', '[', 'I', 'N', 'F', 'O', ']', ' ',
        static_cast<char>(0xEA), static_cast<char>(0xB0),
        static_cast<char>(0x80), '=', '1', '0', '\n'};
    clockState.current = 10;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        utf8Format,
        10) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        9,
        cms::util::StringView(utf8Expected, sizeof(utf8Expected)));

    char localFormat[] = "%s=%d";
    char localArgument[] = "before";
    clockState.current = 11;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        localFormat,
        localArgument,
        11) == cms::util::Status::ok);
    localFormat[0] = 'X';
    localArgument[0] = 'X';
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 10, "[11] [INFO] before=11\n");
    CMS_TEST_CHECK(!sinkState.failed);
    CMS_TEST_CHECK(clockState.calls == 11);

    using SmallLogger = cms::util::log::AsyncLogger<
        16,
        2,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex>;
    ClockState smallClock;
    SinkState smallSink;
    SmallLogger small{
        CountingClock(smallClock),
        CapturingSink(smallSink)};

    smallClock.current = 20;
    CMS_TEST_CHECK(cms::util::log::logf(
        small,
        cms::util::log::Level::info,
        "%s",
        "123456789012345") == cms::util::Status::ok);
    CMS_TEST_CHECK(smallClock.calls == 1);
    CMS_TEST_CHECK(small.pending() == 1);
    CMS_TEST_CHECK(small.drainOne() == cms::util::Status::ok);
    checkLine(smallSink, 0, "[20] [INFO] 123456789012345\n");

    CMS_TEST_CHECK(cms::util::log::logf(
        small,
        cms::util::log::Level::info,
        "%s",
        "1234567890123456") == cms::util::Status::no_space);
    CMS_TEST_CHECK(cms::util::log::logf(
        small,
        cms::util::log::Level::info,
        "%s",
        "a message that is much too long for this logger")
        == cms::util::Status::no_space);
    CMS_TEST_CHECK(cms::util::log::logf(
        small,
        cms::util::log::Level::info,
        static_cast<const char*>(nullptr))
        == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(smallClock.calls == 1);
    CMS_TEST_CHECK(small.pending() == 0);
    CMS_TEST_CHECK(!smallSink.failed);

    return cms::test::finish();
}
