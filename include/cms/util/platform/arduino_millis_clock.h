#pragma once

#include <Arduino.h>

#include <cms/util/log/clock.h>

namespace cms {
namespace util {
namespace platform {

// millis()의 현재 값을 widen할 뿐 rollover를 연장하지 않는다.
class ArduinoMillisClock {
public:
    log::Timestamp nowMilliseconds() noexcept {
        return static_cast<log::Timestamp>(::millis());
    }
};

} // namespace platform
} // namespace util
} // namespace cms
