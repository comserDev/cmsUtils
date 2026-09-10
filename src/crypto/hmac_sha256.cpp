#include <cms/util/crypto/hmac_sha256.h>

#include "sha256_detail.h"

namespace cms {
namespace util {
namespace crypto {
namespace {

constexpr std::size_t Sha256BlockSize = 64;
constexpr std::uint8_t InnerPadByte = 0x36U;
constexpr std::uint8_t OuterPadByte = 0x5CU;

void copyBytes(
    std::uint8_t* destination,
    const std::uint8_t* source,
    std::size_t size) noexcept {
    for (std::size_t i = 0; i < size; ++i) {
        destination[i] = source[i];
    }
}

} // namespace

Status hmacSha256(
    ByteView key,
    ByteView data,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept {
    if ((key.size() != 0 && key.data() == nullptr)
        || (data.size() != 0 && data.data() == nullptr)) {
        return Status::invalid_argument;
    }

    std::uint8_t keyBlock[Sha256BlockSize] = {};
    if (key.size() > Sha256BlockSize) {
        detail::Sha256Context keyContext{};
        detail::initializeSha256(keyContext);
        detail::updateSha256(keyContext, key);

        std::uint8_t reducedKey[Sha256DigestSize] = {};
        detail::finalizeSha256(keyContext, reducedKey);
        copyBytes(keyBlock, reducedKey, Sha256DigestSize);
    } else {
        copyBytes(keyBlock, key.data(), key.size());
    }

    std::uint8_t innerPad[Sha256BlockSize] = {};
    std::uint8_t outerPad[Sha256BlockSize] = {};
    for (std::size_t i = 0; i < Sha256BlockSize; ++i) {
        innerPad[i] = static_cast<std::uint8_t>(keyBlock[i] ^ InnerPadByte);
        outerPad[i] = static_cast<std::uint8_t>(keyBlock[i] ^ OuterPadByte);
    }

    detail::Sha256Context innerContext{};
    detail::initializeSha256(innerContext);
    detail::updateSha256(innerContext, ByteView(innerPad));
    detail::updateSha256(innerContext, data);

    std::uint8_t innerDigest[Sha256DigestSize] = {};
    detail::finalizeSha256(innerContext, innerDigest);

    detail::Sha256Context outerContext{};
    detail::initializeSha256(outerContext);
    detail::updateSha256(outerContext, ByteView(outerPad));
    detail::updateSha256(outerContext, ByteView(innerDigest));

    std::uint8_t result[Sha256DigestSize] = {};
    detail::finalizeSha256(outerContext, result);
    copyBytes(digest, result, Sha256DigestSize);
    return Status::ok;
}

} // namespace crypto
} // namespace util
} // namespace cms
