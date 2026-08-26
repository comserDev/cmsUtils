#include <cstddef>
#include <cstdio>
#include <type_traits>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/record.h>
#include <cms/util/platform/std_mutex.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/platform/steady_clock.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/synchronized_queue.h>

#include "test.h"

namespace {

struct FixedClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 7; }
};

struct SinkState {
    cms::util::StaticString<64> line;
    std::size_t writes = 0;
};

struct CapturingSink {
    explicit CapturingSink(SinkState& state) noexcept
        : state_(&state) {}

    void write(cms::util::StringView text) noexcept {
        ++state_->writes;
        (void)state_->line.assign(text);
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

} // namespace

int main() {
    static_assert(std::is_default_constructible<
        cms::util::platform::StdMutex>::value,
        "StdMutex must be default constructible");
    static_assert(!std::is_copy_constructible<
        cms::util::platform::StdMutex>::value,
        "StdMutex copy must be deleted");
    static_assert(!std::is_move_constructible<
        cms::util::platform::StdMutex>::value,
        "StdMutex move must be deleted");

    cms::util::platform::StdMutex mutex;
    mutex.lock();
    mutex.unlock();

    using Queue = cms::util::StaticQueue<int, 2>;
    cms::util::sync::SynchronizedQueue<Queue, cms::util::platform::StdMutex> queue;
    CMS_TEST_CHECK(queue.push(11) == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.size() == 1);
    int consumed = 0;
    CMS_TEST_CHECK(queue.consumeFront([&consumed](const int& value) {
        consumed = value;
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(consumed == 11);
    CMS_TEST_CHECK(queue.empty());

    cms::util::platform::SteadyClock clock;
    const cms::util::log::Timestamp first = clock.nowMilliseconds();
    const cms::util::log::Timestamp second = clock.nowMilliseconds();
    CMS_TEST_CHECK(second >= first);

    cms::util::platform::StdoutSink stdoutSink;
    stdoutSink.write(cms::util::StringView());

    using Logger = cms::util::log::AsyncLogger<
        16,
        2,
        FixedClock,
        CapturingSink,
        cms::util::platform::StdMutex>;
    SinkState sinkState;
    Logger logger{FixedClock(), CapturingSink(sinkState)};
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, "host")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(sinkState.writes == 1);
    checkBytes(sinkState.line.view(), "[7] [INFO] host\n");

    std::printf(
        "sizeof(cms::util::platform::StdMutex)=%zu\n",
        sizeof(cms::util::platform::StdMutex));
    std::printf(
        "sizeof(cms::util::platform::SteadyClock)=%zu\n",
        sizeof(cms::util::platform::SteadyClock));
    std::printf(
        "sizeof(cms::util::platform::StdoutSink)=%zu\n",
        sizeof(cms::util::platform::StdoutSink));

    return cms::test::finish();
}
