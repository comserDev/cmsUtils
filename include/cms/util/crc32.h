#pragma once

#include <cstdint>

#include <cms/util/byte_view.h>

namespace cms {
namespace util {
namespace crc32 {

class IsoHdlc {
public:
    constexpr IsoHdlc() noexcept : state_(0xFFFFFFFFU) {}

    void reset() noexcept { state_ = 0xFFFFFFFFU; }

    void update(ByteView data) noexcept {
        for (std::size_t i = 0; i < data.size(); ++i) {
            state_ ^= data[i];
            for (unsigned int bit = 0; bit < 8; ++bit) {
                state_ = (state_ >> 1) ^
                    ((state_ & 1U) != 0U ? 0xEDB88320U : 0U);
            }
        }
    }

    std::uint32_t value() const noexcept { return state_ ^ 0xFFFFFFFFU; }

private:
    std::uint32_t state_;
};

inline std::uint32_t isoHdlc(ByteView data) noexcept {
    IsoHdlc crc;
    crc.update(data);
    return crc.value();
}

} // namespace crc32
} // namespace util
} // namespace cms
