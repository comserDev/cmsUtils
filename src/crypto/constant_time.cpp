#include <cms/util/crypto/constant_time.h>

#include <cstddef>
#include <cstdint>

namespace cms {
namespace util {
namespace crypto {

bool constantTimeEqual(
    ByteView lhs,
    ByteView rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    std::uint8_t difference = 0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        difference = static_cast<std::uint8_t>(
            difference | static_cast<std::uint8_t>(lhs[i] ^ rhs[i]));
    }
    return difference == 0;
}

} // namespace crypto
} // namespace util
} // namespace cms
