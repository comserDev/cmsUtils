#include <cstddef>
#include <utility>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/std_queue_async_logger.h>
#include <cms/util/log/tee_sink.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

struct FixedClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 7; }
};

struct SinkState {
    cms::util::StaticString<128> value;
    cms::util::Status result = cms::util::Status::ok;
    std::size_t calls = 0;
};

class StateSink {
public:
    explicit StateSink(SinkState& state) noexcept : state_(&state) {}

    StateSink(const StateSink&) = delete;
    StateSink& operator=(const StateSink&) = delete;
    StateSink(StateSink&&) noexcept = default;
    StateSink& operator=(StateSink&&) noexcept = default;

    cms::util::Status write(cms::util::StringView value) noexcept {
        ++state_->calls;
        if (state_->value.assign(value).status != cms::util::Status::ok) {
            return cms::util::Status::io_error;
        }
        return state_->result;
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

template<class Logger>
void checkFailurePropagation(Logger& logger, SinkState& state) {
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "failure")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.pending() == 1);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::io_error);
    CMS_TEST_CHECK(state.calls == 1);
    CMS_TEST_CHECK(logger.pending() == 0);
    CMS_TEST_CHECK(logger.empty());
}

} // namespace

int main() {
    using StaticLogger = cms::util::log::AsyncLogger<
        32,
        2,
        FixedClock,
        StateSink,
        cms::util::sync::NullMutex>;
    SinkState staticState;
    staticState.result = cms::util::Status::io_error;
    StaticLogger staticLogger{FixedClock(), StateSink(staticState)};
    checkFailurePropagation(staticLogger, staticState);

    using DynamicLogger = cms::util::log::StdQueueAsyncLogger<
        32,
        FixedClock,
        StateSink,
        cms::util::sync::NullMutex>;
    SinkState dynamicState;
    dynamicState.result = cms::util::Status::io_error;
    DynamicLogger dynamicLogger{FixedClock(), StateSink(dynamicState)};
    checkFailurePropagation(dynamicLogger, dynamicState);

    const char payload[] = {'A', '\0', 'B'};
    SinkState first;
    SinkState second;
    cms::util::log::TeeSink<StateSink, StateSink> bothSuccess{
        StateSink(first), StateSink(second)};
    CMS_TEST_CHECK(bothSuccess.write(
        cms::util::StringView(payload, sizeof(payload)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(first.calls == 1);
    CMS_TEST_CHECK(second.calls == 1);
    checkBytes(first.value.view(), cms::util::StringView(payload, sizeof(payload)));
    checkBytes(second.value.view(), cms::util::StringView(payload, sizeof(payload)));

    SinkState firstFailure;
    firstFailure.result = cms::util::Status::io_error;
    SinkState secondSuccess;
    cms::util::log::TeeSink<StateSink, StateSink> firstFails{
        StateSink(firstFailure), StateSink(secondSuccess)};
    CMS_TEST_CHECK(firstFails.write("first") == cms::util::Status::io_error);
    CMS_TEST_CHECK(firstFailure.calls == 1);
    CMS_TEST_CHECK(secondSuccess.calls == 1);

    SinkState firstSuccess;
    SinkState secondFailure;
    secondFailure.result = cms::util::Status::unsupported;
    cms::util::log::TeeSink<StateSink, StateSink> secondFails{
        StateSink(firstSuccess), StateSink(secondFailure)};
    CMS_TEST_CHECK(secondFails.write("second")
        == cms::util::Status::unsupported);
    CMS_TEST_CHECK(firstSuccess.calls == 1);
    CMS_TEST_CHECK(secondFailure.calls == 1);

    SinkState bothFirstFailure;
    bothFirstFailure.result = cms::util::Status::io_error;
    SinkState bothSecondFailure;
    bothSecondFailure.result = cms::util::Status::unsupported;
    cms::util::log::TeeSink<StateSink, StateSink> bothFail{
        StateSink(bothFirstFailure), StateSink(bothSecondFailure)};
    CMS_TEST_CHECK(bothFail.write("both") == cms::util::Status::io_error);
    CMS_TEST_CHECK(bothFirstFailure.calls == 1);
    CMS_TEST_CHECK(bothSecondFailure.calls == 1);

    SinkState loggerFirst;
    SinkState loggerSecond;
    using LoggerTee = cms::util::log::TeeSink<StateSink, StateSink>;
    using TeeLogger = cms::util::log::AsyncLogger<
        32,
        2,
        FixedClock,
        LoggerTee,
        cms::util::sync::NullMutex>;
    LoggerTee tee{StateSink(loggerFirst), StateSink(loggerSecond)};
    TeeLogger teeLogger{FixedClock(), std::move(tee)};
    CMS_TEST_CHECK(teeLogger.log(cms::util::log::Level::warning, "tee")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(teeLogger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(loggerFirst.calls == 1);
    CMS_TEST_CHECK(loggerSecond.calls == 1);
    checkBytes(loggerFirst.value.view(), "[7] [WARNING] tee\n");
    checkBytes(loggerSecond.value.view(), "[7] [WARNING] tee\n");

    return cms::test::finish();
}
