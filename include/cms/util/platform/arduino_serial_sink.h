#pragma once

#include <cstdint>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace platform {

// Serial-like object를 소유하지 않으므로 sink보다 오래 살아야 한다.
// C string 대신 length 기반 write를 사용해 embedded NUL도 보존한다.
template<class Serial>
class ArduinoSerialSink {
public:
    // serial object를 소유하지 않는 sink를 만든다.
    explicit ArduinoSerialSink(Serial& serial) noexcept
        : serial_(&serial) {}

    // text의 길이를 그대로 사용해 Serial에 기록한다.
    Status write(StringView text) {
        if (text.empty()) {
            return Status::ok;
        }

        return serial_->write(
                   reinterpret_cast<const std::uint8_t*>(text.data()),
                   text.size()) == text.size()
            ? Status::ok
            : Status::io_error;
    }

private:
    Serial* serial_;
};

} // namespace platform
} // namespace util
} // namespace cms
