#include <Arduino.h>

#include <cms/util/binary_reader.h>
#include <cms/util/binary_writer.h>
#include <cms/util/crc32.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/platform/arduino_millis_clock.h>
#include <cms/util/platform/arduino_serial_sink.h>
#include <cms/util/static_byte_buffer.h>
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

    cms::util::StaticByteBuffer<16> bytes;
    cms::util::BinaryWriter writer(bytes.buffer());
    (void)writer.writeUint32BigEndian(0x12345678U);
    cms::util::BinaryReader reader(bytes.view());
    std::uint32_t value = 0;
    (void)reader.readUint32BigEndian(value);
    volatile std::uint32_t crc = cms::util::crc32::isoHdlc(bytes.view());
    (void)value;
    (void)crc;

    const auto status = logger.log(
        cms::util::log::Level::info,
        "cmsUtils V2 ESP32 smoke");

    if (status == cms::util::Status::ok) {
        (void)logger.drainOne();
    }
}

void loop() {}
