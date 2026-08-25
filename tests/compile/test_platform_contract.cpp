#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/log/async_logger.h>
#include <cms/platform/arduino_serial_sink.h>
#include <cms/platform/std_mutex.h>
#include <cms/platform/stdout_sink.h>
#include <cms/platform/steady_clock.h>

namespace {

struct TestClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct TestSink {
    void write(cms::StringView) noexcept {}
};

struct TestSerial {
    TestSerial() = default;
    TestSerial(const TestSerial&) = delete;
    TestSerial& operator=(const TestSerial&) = delete;

    std::size_t write(const unsigned char*, std::size_t) { return 0; }
};

using StdLogger = cms::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::platform::StdMutex>;
using SerialSink = cms::platform::ArduinoSerialSink<TestSerial>;

} // namespace

static_assert(std::is_default_constructible<cms::platform::StdMutex>::value,
    "StdMutex must be default constructible");
static_assert(!std::is_copy_constructible<cms::platform::StdMutex>::value,
    "StdMutex copy must be deleted");
static_assert(!std::is_move_constructible<cms::platform::StdMutex>::value,
    "StdMutex move must be deleted");
static_assert(std::is_default_constructible<StdLogger>::value,
    "StdMutex must integrate with AsyncLogger");
static_assert(std::is_constructible<StdLogger, TestClock, TestSink>::value,
    "StdMutex logger must support Clock and Sink injection");
static_assert(std::is_constructible<SerialSink, TestSerial&>::value,
    "ArduinoSerialSink must bind a Serial-like reference");
static_assert(std::is_same<
    decltype(std::declval<cms::platform::SteadyClock&>().nowMilliseconds()),
    cms::log::Timestamp>::value,
    "SteadyClock must return Timestamp");
static_assert(std::is_same<
    decltype(std::declval<cms::platform::StdoutSink&>().write(
        cms::StringView())),
    void>::value,
    "StdoutSink write must return void");
