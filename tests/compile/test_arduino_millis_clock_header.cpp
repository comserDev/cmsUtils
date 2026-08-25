#include <cms/platform/arduino_millis_clock.h>

static_assert(
    sizeof(cms::platform::ArduinoMillisClock) > 0,
    "arduino_millis_clock.h must compile with Arduino API");
