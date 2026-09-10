#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/status.h>

namespace cms {
namespace util {
namespace crypto {

constexpr std::size_t Sha256DigestSize = 32;

Status sha256(
    ByteView input,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept;

} // namespace crypto
} // namespace util
} // namespace cms
