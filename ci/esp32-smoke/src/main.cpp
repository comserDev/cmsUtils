#include <Arduino.h>

#include <cms/util/log/async_logger.h>
#include <cms/util/platform/arduino_millis_clock.h>
#include <cms/util/platform/arduino_serial_sink.h>
#include <cms/util/sync/null_mutex.h>

using Sink = cms::util::platform::ArduinoSerialSink<HardwareSerial>;
using Logger = cms::util::log::AsyncLogger<
    64,
    4,
    cms::util::platform::ArduinoMillisClock,
    Sink,
    cms::util::sync::NullMutex>;

Logger logger{
    cms::util::platform::ArduinoMillisClock{},
    Sink{Serial}};

void setup() {
    Serial.begin(115200);

    const auto status = logger.log(
        cms::util::log::Level::info,
        "cmsUtils V2 ESP32 smoke");

    if (status == cms::util::Status::ok) {
        (void)logger.drainOne();
    }
}

void loop() {}
