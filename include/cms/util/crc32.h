#pragma once

#include <cstdint>

#include <cms/util/byte_view.h>

namespace cms {
namespace util {
namespace crc32 {

// CRC-32/ISO-HDLC 상태를 incremental하게 계산한다. 암호학적 무결성이나
// 인증을 제공하지 않으며, 입력 byte를 순서대로 update해야 한다.
class IsoHdlc {
public:
    // 표준 초기 상태로 시작한다.
    constexpr IsoHdlc() noexcept : state_(0xFFFFFFFFU) {}

    // 누적 상태를 초기화한다.
    void reset() noexcept { state_ = 0xFFFFFFFFU; }

    // data를 현재 CRC에 이어 반영한다.
    void update(ByteView data) noexcept {
        for (std::size_t i = 0; i < data.size(); ++i) {
            state_ ^= data[i];
            for (unsigned int bit = 0; bit < 8; ++bit) {
                state_ = (state_ >> 1) ^
                    ((state_ & 1U) != 0U ? 0xEDB88320U : 0U);
            }
        }
    }

    // 최종 XOR를 적용한 CRC 값을 반환한다.
    std::uint32_t value() const noexcept { return state_ ^ 0xFFFFFFFFU; }

private:
    std::uint32_t state_;
};

// data 전체의 CRC-32/ISO-HDLC 값을 한 번에 계산한다.
inline std::uint32_t isoHdlc(ByteView data) noexcept {
    IsoHdlc crc;
    crc.update(data);
    return crc.value();
}

} // namespace crc32
} // namespace util
} // namespace cms
