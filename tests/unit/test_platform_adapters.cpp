#include <cstddef>
#include <cstdio>
#include <type_traits>

#include <cms/log/async_logger.h>
#include <cms/log/record.h>
#include <cms/platform/std_mutex.h>
#include <cms/platform/stdout_sink.h>
#include <cms/platform/steady_clock.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/synchronized_queue.h>

#include "test.h"

namespace {

struct FixedClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 7; }
};

struct SinkState {
    cms::StaticString<64> line;
    std::size_t writes = 0;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept
        : state_(&state) {}

    void write(cms::StringView text) noexcept {
        ++state_->writes;
        (void)state_->line.assign(text);
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

} // namespace

int main() {
    static_assert(std::is_default_constructible<
        cms::platform::StdMutex>::value,
        "StdMutex must be default constructible");
    static_assert(!std::is_copy_constructible<
        cms::platform::StdMutex>::value,
        "StdMutex copy must be deleted");
    static_assert(!std::is_move_constructible<
        cms::platform::StdMutex>::value,
        "StdMutex move must be deleted");

    cms::platform::StdMutex mutex;
    mutex.lock();
    mutex.unlock();

    using Queue = cms::StaticQueue<int, 2>;
    cms::SynchronizedQueue<Queue, cms::platform::StdMutex> queue;
    CMS_TEST_CHECK(queue.push(11) == cms::Status::ok);
    CMS_TEST_CHECK(queue.size() == 1);
    int consumed = 0;
    CMS_TEST_CHECK(queue.consumeFront([&consumed](const int& value) {
        consumed = value;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(consumed == 11);
    CMS_TEST_CHECK(queue.empty());

    cms::platform::SteadyClock clock;
    const cms::log::Timestamp first = clock.nowMilliseconds();
    const cms::log::Timestamp second = clock.nowMilliseconds();
    CMS_TEST_CHECK(second >= first);

    cms::platform::StdoutSink stdoutSink;
    stdoutSink.write(cms::StringView());

    using Logger = cms::log::AsyncLogger<
        16,
        2,
        FixedClock,
        CapturingSink,
        cms::platform::StdMutex>;
    SinkState sinkState;
    Logger logger{FixedClock(), CapturingSink(sinkState)};
    CMS_TEST_CHECK(logger.log(cms::log::Level::info, "host")
        == cms::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::Status::ok);
    CMS_TEST_CHECK(sinkState.writes == 1);
    checkBytes(sinkState.line.view(), "[7] [INFO] host\n");

    std::printf(
        "sizeof(cms::platform::StdMutex)=%zu\n",
        sizeof(cms::platform::StdMutex));
    std::printf(
        "sizeof(cms::platform::SteadyClock)=%zu\n",
        sizeof(cms::platform::SteadyClock));
    std::printf(
        "sizeof(cms::platform::StdoutSink)=%zu\n",
        sizeof(cms::platform::StdoutSink));

    return cms::test::finish();
}
