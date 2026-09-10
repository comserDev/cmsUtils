#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/crypto/sha256.h>

namespace cms {
namespace util {
namespace crypto {
namespace detail {

// SHA-256 one-shot 함수와 HMAC 구현이 공유하는 내부 streaming state다.
// Public API가 아니며 caller가 직접 생성하거나 상태를 이어 쓰는 용도로 제공하지 않는다.
struct Sha256Context {
    std::uint32_t state[8];
    std::uint8_t block[64];
    std::uint64_t totalBytes;
    std::size_t buffered;
};

void initializeSha256(Sha256Context& context) noexcept;

void updateSha256(
    Sha256Context& context,
    ByteView input) noexcept;

void finalizeSha256(
    Sha256Context& context,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept;

} // namespace detail
} // namespace crypto
} // namespace util
} // namespace cms
