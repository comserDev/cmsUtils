#include <cms/util/crypto/sha256.h>

int sha256HeaderCompile() {
    std::uint8_t digest[cms::util::crypto::Sha256DigestSize] = {};
    return cms::util::crypto::sha256(cms::util::ByteView(), digest) ==
        cms::util::Status::ok ? 0 : 1;
}
