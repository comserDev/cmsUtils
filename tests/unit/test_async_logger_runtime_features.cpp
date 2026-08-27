#include <cstddef>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

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
    bool captureFailed = false;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept
        : state_(&state) {}

    cms::util::Status write(cms::util::StringView text) noexcept {
        if (state_->writes >= 8
            || state_->lines[state_->writes].assign(text).status
                != cms::util::Status::ok) {
            state_->captureFailed = true;
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
        16,
        4,
        CountingClock,
        CapturingSink,
        cms::util::sync::NullMutex,
        cms::util::log::RuntimeAnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;

    ClockState clockState;
    SinkState sinkState;
    Logger logger{CountingClock(clockState), CapturingSink(sinkState)};

    CMS_TEST_CHECK(logger.useColor());
    CMS_TEST_CHECK(logger.loggingEnabled());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::debug);
    logger.setUseColor(true);
    logger.setMinLevel(cms::util::log::Level::warning);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "hidden-debug")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "hidden-info")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 0);

    clockState.current = 10;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "warning")
        == cms::util::Status::ok);
    clockState.current = 11;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "error")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        0,
        "[10] \033[33m[WARNING]\033[0m warning\n");
    checkLine(
        sinkState,
        1,
        "[11] \033[31m[ERROR]\033[0m error\n");

    logger.setUseColor(false);
    clockState.current = 12;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "critical")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 2, "[12] [CRITICAL] critical\n");

    logger.setMinLevel(cms::util::log::Level::trace);
    logger.setUseColor(false);
    clockState.current = 13;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "queued")
        == cms::util::Status::ok);
    logger.setMinLevel(cms::util::log::Level::critical);
    logger.setUseColor(true);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        3,
        "[13] \033[32m[INFO]\033[0m queued\n");

    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "never")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 4);
    logger.setMinLevel(cms::util::log::Level::trace);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);

    const cms::util::log::Level invalid = static_cast<cms::util::log::Level>(0xFF);
    clockState.current = 14;
    CMS_TEST_CHECK(logger.log(invalid, "invalid") == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        4,
        "[14] \033[0m[UNKNOWN] invalid\n");
    CMS_TEST_CHECK(clockState.calls == 5);

    logger.setUseColor(true);
    logger.setMinLevel(cms::util::log::Level::warning);
    logger.setLoggingEnabled(false);
    CMS_TEST_CHECK(!logger.loggingEnabled());
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "disabled")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.log(invalid, "disabled-invalid")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(clockState.calls == 5);
    CMS_TEST_CHECK(logger.pending() == 0);

    logger.setLoggingEnabled(true);
    CMS_TEST_CHECK(logger.loggingEnabled());
    CMS_TEST_CHECK(logger.minLevel() == cms::util::log::Level::warning);
    clockState.current = 15;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "enabled")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(
        sinkState,
        5,
        "[15] \033[31m[ERROR]\033[0m enabled\n");

    logger.setUseColor(false);
    clockState.current = 16;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::critical, "plain")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sinkState, 6, "[16] [CRITICAL] plain\n");
    CMS_TEST_CHECK(clockState.calls == 7);
    CMS_TEST_CHECK(!sinkState.captureFailed);

    return cms::test::finish();
}
