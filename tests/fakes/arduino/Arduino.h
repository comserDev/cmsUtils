#pragma once

namespace cms_test_arduino {

inline unsigned long currentMillis = 0;

inline void setMillis(unsigned long value) noexcept {
    currentMillis = value;
}

} // namespace cms_test_arduino

inline unsigned long millis() noexcept {
    return cms_test_arduino::currentMillis;
}
