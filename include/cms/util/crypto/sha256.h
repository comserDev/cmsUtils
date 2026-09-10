#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/status.h>

namespace cms {
namespace util {
namespace crypto {

constexpr std::size_t Sha256DigestSize = 32;

// input 전체의 SHA-256 digest를 32 byte binary output에 기록한다.
// 성공할 때만 digest를 갱신하며, heap이나 문자열 인코딩은 사용하지 않는다.
Status sha256(
    ByteView input,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept;

} // namespace crypto
} // namespace util
} // namespace cms
