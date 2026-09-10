#include <cms/util/crypto/hmac_sha256.h>

int hmacSha256HeaderCompile() {
    std::uint8_t digest[cms::util::crypto::Sha256DigestSize] = {};
    return cms::util::crypto::hmacSha256(
        cms::util::ByteView(),
        cms::util::ByteView(),
        digest) == cms::util::Status::ok ? 0 : 1;
}
