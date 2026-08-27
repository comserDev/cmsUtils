#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <cms/util/platform/arduino_millis_clock.h>
#include <cms/util/platform/arduino_serial_sink.h>
#include <cms/util/platform/freertos_static_mutex.h>
#include <cms/util/static_queue.h>
#include <cms/util/sync/synchronized_queue.h>

#include "test.h"

namespace {

struct FakeSerial {
    FakeSerial() = default;
    FakeSerial(const FakeSerial&) = delete;
    FakeSerial& operator=(const FakeSerial&) = delete;

    std::size_t write(const std::uint8_t* data, std::size_t size) noexcept {
        ++calls;
        receivedSize = size;
        for (std::size_t index = 0; index < size && index < 16; ++index) {
            bytes[index] = data[index];
        }
        return shortWrite ? size - 1 : size;
    }

    std::uint8_t bytes[16] = {};
    std::size_t receivedSize = 0;
    std::size_t calls = 0;
    bool shortWrite = false;
};

} // namespace

int main() {
    FakeSerial serial;
    cms::util::platform::ArduinoSerialSink<FakeSerial> serialSink(serial);
    const char payload[] = {'A', '\0', 'B'};
    CMS_TEST_CHECK(serialSink.write(
        cms::util::StringView(payload, sizeof(payload)))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(serial.calls == 1);
    CMS_TEST_CHECK(serial.receivedSize == sizeof(payload));
    CMS_TEST_CHECK(serial.bytes[0] == 'A');
    CMS_TEST_CHECK(serial.bytes[1] == 0);
    CMS_TEST_CHECK(serial.bytes[2] == 'B');
    CMS_TEST_CHECK(serialSink.write(cms::util::StringView())
        == cms::util::Status::ok);
    CMS_TEST_CHECK(serial.calls == 1);
    serial.shortWrite = true;
    CMS_TEST_CHECK(serialSink.write("x") == cms::util::Status::io_error);
    CMS_TEST_CHECK(serial.calls == 2);

    cms::util::platform::ArduinoMillisClock millisClock;
    cms_test_arduino::setMillis(0);
    CMS_TEST_CHECK(millisClock.nowMilliseconds() == 0);
    cms_test_arduino::setMillis(123456UL);
    CMS_TEST_CHECK(millisClock.nowMilliseconds() == 123456);
    cms_test_arduino::setMillis(
        (std::numeric_limits<std::uint32_t>::max)());
    CMS_TEST_CHECK(
        millisClock.nowMilliseconds()
        == (std::numeric_limits<std::uint32_t>::max)());

    static_assert(std::is_default_constructible<
        cms::util::platform::FreeRtosStaticMutex>::value,
        "FreeRtosStaticMutex must be default constructible");
    static_assert(!std::is_copy_constructible<
        cms::util::platform::FreeRtosStaticMutex>::value,
        "FreeRtosStaticMutex copy must be deleted");
    static_assert(!std::is_move_constructible<
        cms::util::platform::FreeRtosStaticMutex>::value,
        "FreeRtosStaticMutex move must be deleted");

    cms_test_freertos::reset();
    {
        cms::util::platform::FreeRtosStaticMutex mutex;
        CMS_TEST_CHECK(cms_test_freertos::createCalls == 1);
        CMS_TEST_REQUIRE(cms_test_freertos::createdStorage != nullptr);
        mutex.lock();
        CMS_TEST_CHECK(cms_test_freertos::takeCalls == 1);
        CMS_TEST_CHECK(
            cms_test_freertos::takenHandle
            == cms_test_freertos::createdStorage);
        CMS_TEST_CHECK(cms_test_freertos::takeTicks == portMAX_DELAY);
        mutex.unlock();
        CMS_TEST_CHECK(cms_test_freertos::giveCalls == 1);
        CMS_TEST_CHECK(
            cms_test_freertos::givenHandle
            == cms_test_freertos::createdStorage);
    }
    CMS_TEST_CHECK(cms_test_freertos::deleteCalls == 1);
    CMS_TEST_CHECK(
        cms_test_freertos::deletedHandle
        == cms_test_freertos::createdStorage);

    cms_test_freertos::reset();
    cms_test_freertos::takeFailuresBeforeSuccess = 1;
    {
        cms::util::platform::FreeRtosStaticMutex retryingMutex;
        retryingMutex.lock();
        CMS_TEST_CHECK(cms_test_freertos::takeCalls == 2);
        CMS_TEST_CHECK(
            cms_test_freertos::takeFailuresBeforeSuccess == 0);
        CMS_TEST_CHECK(
            cms_test_freertos::takenHandle
            == cms_test_freertos::createdStorage);
        retryingMutex.unlock();
        CMS_TEST_CHECK(cms_test_freertos::giveCalls == 1);
    }
    CMS_TEST_CHECK(cms_test_freertos::deleteCalls == 1);

    cms_test_freertos::reset();
    using Queue = cms::util::StaticQueue<int, 1>;
    {
        cms::util::sync::SynchronizedQueue<
            Queue,
            cms::util::platform::FreeRtosStaticMutex> queue;
        CMS_TEST_CHECK(queue.push(9) == cms::util::Status::ok);
        CMS_TEST_CHECK(queue.full());
        CMS_TEST_CHECK(queue.pop() == cms::util::Status::ok);
        CMS_TEST_CHECK(queue.empty());
    }
    CMS_TEST_CHECK(cms_test_freertos::createCalls == 1);
    CMS_TEST_CHECK(cms_test_freertos::takeCalls == 4);
    CMS_TEST_CHECK(cms_test_freertos::giveCalls == 4);
    CMS_TEST_CHECK(cms_test_freertos::deleteCalls == 1);

    return cms::test::finish();
}
