#pragma once

#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/crypto/sha256.h>
#include <cms/util/status.h>

namespace cms {
namespace util {
namespace crypto {

// key와 data로 HMAC-SHA256 digest를 계산한다. SHA-256 block보다 긴 key는 먼저
// SHA-256으로 줄여 사용하며, 성공한 경우에만 caller의 32-byte digest를 갱신한다.
// Empty key/data와 임의 binary byte를 허용하고 동적 메모리는 사용하지 않는다.
Status hmacSha256(
    ByteView key,
    ByteView data,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept;

} // namespace crypto
} // namespace util
} // namespace cms
