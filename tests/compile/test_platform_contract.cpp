#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/util/log/async_logger.h>
#include <cms/util/platform/arduino_serial_sink.h>
#include <cms/util/platform/std_mutex.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/platform/steady_clock.h>

namespace {

struct TestClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct TestSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
};

struct TestSerial {
    TestSerial() = default;
    TestSerial(const TestSerial&) = delete;
    TestSerial& operator=(const TestSerial&) = delete;

    std::size_t write(const unsigned char*, std::size_t) { return 0; }
};

using StdLogger = cms::util::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::util::platform::StdMutex>;
using SerialSink = cms::util::platform::ArduinoSerialSink<TestSerial>;

} // namespace

static_assert(std::is_default_constructible<cms::util::platform::StdMutex>::value,
    "StdMutex must be default constructible");
static_assert(!std::is_copy_constructible<cms::util::platform::StdMutex>::value,
    "StdMutex copy must be deleted");
static_assert(!std::is_move_constructible<cms::util::platform::StdMutex>::value,
    "StdMutex move must be deleted");
static_assert(std::is_default_constructible<StdLogger>::value,
    "StdMutex must integrate with AsyncLogger");
static_assert(std::is_constructible<StdLogger, TestClock, TestSink>::value,
    "StdMutex logger must support Clock and Sink injection");
static_assert(std::is_constructible<SerialSink, TestSerial&>::value,
    "ArduinoSerialSink must bind a Serial-like reference");
static_assert(std::is_same<
    decltype(std::declval<cms::util::platform::SteadyClock&>().nowMilliseconds()),
    cms::util::log::Timestamp>::value,
    "SteadyClock must return Timestamp");
static_assert(std::is_same<
    decltype(std::declval<cms::util::platform::StdoutSink&>().write(
        cms::util::StringView())),
    cms::util::Status>::value,
    "StdoutSink write must return Status");
static_assert(std::is_same<
    decltype(std::declval<SerialSink&>().write(cms::util::StringView())),
    cms::util::Status>::value,
    "ArduinoSerialSink write must return Status");
