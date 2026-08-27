#pragma once

#include <cstdint>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace platform {

// UDP-like object를 소유하지 않으므로 sink보다 오래 살아야 한다.
// Socket 초기화와 local port 관리는 application이 담당한다.
template<class Udp, class Address>
class ArduinoUdpSink {
public:
    ArduinoUdpSink(
        Udp& udp,
        Address remoteAddress,
        std::uint16_t remotePort)
        : udp_(&udp),
          remoteAddress_(remoteAddress),
          remotePort_(remotePort) {}

    Status write(StringView data) {
        if (data.empty()) {
            return Status::ok;
        }

        if (!udp_->beginPacket(remoteAddress_, remotePort_)) {
            return Status::io_error;
        }

        const bool exactWrite = udp_->write(
            reinterpret_cast<const std::uint8_t*>(data.data()),
            data.size()) == data.size();
        const bool packetEnded = udp_->endPacket() != 0;
        return exactWrite && packetEnded ? Status::ok : Status::io_error;
    }

private:
    Udp* udp_;
    Address remoteAddress_;
    std::uint16_t remotePort_;
};

} // namespace platform
} // namespace util
} // namespace cms
