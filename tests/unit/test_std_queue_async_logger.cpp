#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>

#include <cms/util/log/printf_log.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/std_queue_async_logger.h>
#include <cms/util/log/utc_offset_formatter.h>
#include <cms/util/platform/std_mutex.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/null_mutex.h>

#include "test.h"

namespace {

struct ClockState {
    cms::util::log::Timestamp current = 0;
    std::size_t calls = 0;
};

struct TestClock {
    explicit TestClock(ClockState& state) noexcept : state_(&state) {}

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
    bool failed = false;
};

struct TestSink {
    explicit TestSink(SinkState& state) noexcept : state_(&state) {}

    void write(cms::util::StringView text) noexcept {
        if (state_->writes >= 16
            || state_->lines[state_->writes].assign(text).status
                != cms::util::Status::ok) {
            state_->failed = true;
            return;
        }
        ++state_->writes;
    }

private:
    SinkState* state_;
};

struct GrowthClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept {
        return next++;
    }

    cms::util::log::Timestamp next = 0;
};

struct GrowthSink {
    void write(cms::util::StringView) noexcept {
        ++writes;
    }

    static std::size_t writes;
};

std::size_t GrowthSink::writes = 0;

struct AtomicClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept {
        return next.fetch_add(1, std::memory_order_relaxed);
    }

    static std::atomic<cms::util::log::Timestamp> next;
};

std::atomic<cms::util::log::Timestamp> AtomicClock::next{0};

struct AtomicSink {
    void write(cms::util::StringView) noexcept {
        ++writes;
    }

    static std::size_t writes;
};

std::size_t AtomicSink::writes = 0;

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
    using Logger = cms::util::log::StdQueueAsyncLogger<
        32,
        TestClock,
        TestSink,
        cms::util::sync::NullMutex>;

    ClockState clock;
    SinkState sink;
    Logger logger{TestClock(clock), TestSink(sink)};

    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(logger.pending() == 0);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::out_of_range);

    char owned[] = "owned";
    clock.current = 10;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::info, owned)
        == cms::util::Status::ok);
    owned[0] = 'X';
    CMS_TEST_CHECK(logger.pending() == 1);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sink, 0, "[10] [INFO] owned\n");
    CMS_TEST_CHECK(logger.empty());

    clock.current = 20;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::debug, "A")
        == cms::util::Status::ok);
    clock.current = 21;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::warning, "B")
        == cms::util::Status::ok);
    clock.current = 22;
    CMS_TEST_CHECK(logger.log(cms::util::log::Level::error, "C")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.pending() == 3);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sink, 1, "[20] [DEBUG] A\n");
    checkLine(sink, 2, "[21] [WARNING] B\n");
    checkLine(sink, 3, "[22] [ERROR] C\n");

    const char embedded[] = {'A', '\0', 'B'};
    clock.current = 30;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::info,
        cms::util::StringView(embedded, sizeof(embedded)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    const char embeddedExpected[] = {
        '[', '3', '0', ']', ' ', '[', 'I', 'N', 'F', 'O', ']', ' ',
        'A', '\0', 'B', '\n'};
    checkLine(
        sink,
        4,
        cms::util::StringView(embeddedExpected, sizeof(embeddedExpected)));

    char oversized[32] = {};
    const std::size_t clockCallsBeforeOversized = clock.calls;
    CMS_TEST_CHECK(logger.log(
        cms::util::log::Level::info,
        cms::util::StringView(oversized, sizeof(oversized)))
        == cms::util::Status::no_space);
    CMS_TEST_CHECK(clock.calls == clockCallsBeforeOversized);
    CMS_TEST_CHECK(logger.empty());
    CMS_TEST_CHECK(!sink.failed);

    using RuntimeLogger = cms::util::log::StdQueueAsyncLogger<
        32,
        TestClock,
        TestSink,
        cms::util::sync::NullMutex,
        cms::util::log::RuntimeAnsiFormatter,
        cms::util::log::RuntimeLevelFilter>;
    ClockState runtimeClock;
    SinkState runtimeSink;
    RuntimeLogger runtime{
        TestClock(runtimeClock), TestSink(runtimeSink)};
    runtime.setMinLevel(cms::util::log::Level::warning);
    runtimeClock.current = 40;
    CMS_TEST_CHECK(runtime.log(cms::util::log::Level::info, "skip")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(runtimeClock.calls == 0);
    CMS_TEST_CHECK(runtime.pending() == 0);
    CMS_TEST_CHECK(runtime.log(cms::util::log::Level::warning, "color")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(runtimeClock.calls == 1);
    CMS_TEST_CHECK(runtime.drainOne() == cms::util::Status::ok);
    checkLine(runtimeSink, 0, "[40] \033[33m[WARNING]\033[0m color\n");
    runtime.setUseColor(false);
    CMS_TEST_CHECK(!runtime.useColor());
    runtimeClock.current = 41;
    CMS_TEST_CHECK(runtime.log(cms::util::log::Level::error, "plain")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(runtime.drainOne() == cms::util::Status::ok);
    checkLine(runtimeSink, 1, "[41] [ERROR] plain\n");

    using UtcLogger = cms::util::log::StdQueueAsyncLogger<
        32,
        TestClock,
        TestSink,
        cms::util::sync::NullMutex,
        cms::util::log::UtcOffsetFormatter<cms::util::log::PlainFormatter>>;
    ClockState utcClock;
    SinkState utcSink;
    UtcLogger utc{TestClock(utcClock), TestSink(utcSink)};
    CMS_TEST_CHECK(utc.setUtcOffsetMinutes(330) == cms::util::Status::ok);
    CMS_TEST_CHECK(utc.log(cms::util::log::Level::info, "india")
        == cms::util::Status::ok);
    CMS_TEST_CHECK(utc.setUtcOffsetMinutes(345) == cms::util::Status::ok);
    CMS_TEST_CHECK(utc.drainOne() == cms::util::Status::ok);
    checkLine(utcSink, 0, "[05:45:00] [INFO] india\n");

    clock.current = 50;
    CMS_TEST_CHECK(cms::util::log::logf(
        logger,
        cms::util::log::Level::info,
        "value=%d",
        7) == cms::util::Status::ok);
    CMS_TEST_CHECK(logger.drainOne() == cms::util::Status::ok);
    checkLine(sink, 5, "[50] [INFO] value=7\n");

    using GrowthLogger = cms::util::log::StdQueueAsyncLogger<
        24,
        GrowthClock,
        GrowthSink,
        cms::util::sync::NullMutex>;
    GrowthSink::writes = 0;
    GrowthLogger growth;
    for (std::size_t index = 0; index < 1000; ++index) {
        CMS_TEST_REQUIRE(growth.log(cms::util::log::Level::info, "growth")
            == cms::util::Status::ok);
    }
    CMS_TEST_CHECK(growth.pending() == 1000);
    for (std::size_t index = 0; index < 1000; ++index) {
        CMS_TEST_REQUIRE(growth.drainOne() == cms::util::Status::ok);
    }
    CMS_TEST_CHECK(growth.empty());
    CMS_TEST_CHECK(GrowthSink::writes == 1000);

    using ConcurrentLogger = cms::util::log::StdQueueAsyncLogger<
        24,
        AtomicClock,
        AtomicSink,
        cms::util::platform::StdMutex>;
    AtomicClock::next.store(0, std::memory_order_relaxed);
    AtomicSink::writes = 0;
    ConcurrentLogger concurrent;
    std::atomic<std::size_t> producerFailures{0};
    constexpr std::size_t producerCount = 4;
    constexpr std::size_t recordsPerProducer = 100;
    std::thread producers[producerCount];
    for (std::size_t producer = 0; producer < producerCount; ++producer) {
        producers[producer] = std::thread(
            [&concurrent, &producerFailures]() {
            for (std::size_t record = 0;
                 record < recordsPerProducer;
                 ++record) {
                if (concurrent.log(
                    cms::util::log::Level::info,
                    "concurrent") != cms::util::Status::ok) {
                    producerFailures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& producer : producers) {
        producer.join();
    }
    CMS_TEST_CHECK(producerFailures.load(std::memory_order_relaxed) == 0);
    CMS_TEST_CHECK(concurrent.pending()
        == producerCount * recordsPerProducer);
    while (!concurrent.empty()) {
        CMS_TEST_REQUIRE(concurrent.drainOne() == cms::util::Status::ok);
    }
    CMS_TEST_CHECK(AtomicSink::writes
        == producerCount * recordsPerProducer);

    std::printf("sizeof(StdQueueAsyncLogger<32, ...>)=%zu\n", sizeof(Logger));
    return cms::test::finish();
}
