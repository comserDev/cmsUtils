#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/tee_sink.h>
#include <cms/util/platform/arduino_millis_clock.h>
#include <cms/util/platform/arduino_serial_sink.h>
#include <cms/util/platform/arduino_udp_sink.h>
#include <cms/util/sync/null_mutex.h>

namespace {

constexpr std::uint16_t localPort = 40000;
constexpr std::uint16_t remotePort = 40001;

WiFiUDP udp;
IPAddress remoteAddress(192, 168, 0, 10);

using SerialSink = cms::util::platform::ArduinoSerialSink<HardwareSerial>;
using UdpSink = cms::util::platform::ArduinoUdpSink<WiFiUDP, IPAddress>;
using OutputSink = cms::util::log::TeeSink<SerialSink, UdpSink>;
// setup/loop 단일-thread 예제이므로 NullMutex를 사용한다.
using Logger = cms::util::log::AsyncLogger<
    128,
    8,
    cms::util::platform::ArduinoMillisClock,
    OutputSink,
    cms::util::sync::NullMutex>;

Logger logger{
    cms::util::platform::ArduinoMillisClock{},
    OutputSink{SerialSink{Serial}, UdpSink{udp, remoteAddress, remotePort}}};

} // namespace

void setup() {
    Serial.begin(115200);

    // WiFi 연결을 완료한 뒤 udp.begin()을 호출하는 것은 application 책임이다.
    // ArduinoUdpSink는 begin/stop을 호출하지 않으므로 UDP lifecycle도 여기서 관리한다.
    if (!udp.begin(localPort)) {
        Serial.println("UDP begin failed");
        return;
    }

    const auto logStatus = logger.log(cms::util::log::Level::info, "Serial and UDP ready");
    if (logStatus != cms::util::Status::ok) {
        Serial.println("log enqueue failed");
        return;
    }

    const auto outputStatus = logger.drainOne();
    if (outputStatus != cms::util::Status::ok) {
        Serial.println("log output failed");
    }
}

void loop() {
    // 새 record를 enqueue한 application 지점에서 drainOne()을 호출한다.
}
