#include <cms/util/platform/arduino_serial_sink.h>

struct HeaderSerial {
    void write(const unsigned char*, decltype(sizeof(0))) {}
};

using HeaderSink = cms::util::platform::ArduinoSerialSink<HeaderSerial>;

static_assert(
    sizeof(HeaderSink) > 0,
    "arduino_serial_sink.h must compile independently");
