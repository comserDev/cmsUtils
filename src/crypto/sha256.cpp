#include <cms/util/crypto/sha256.h>

namespace cms {
namespace util {
namespace crypto {
namespace {

struct Sha256Context {
    std::uint32_t state[8];
    std::uint8_t block[64];
    std::uint64_t totalBytes;
    std::size_t buffered;
};

constexpr std::uint32_t roundConstants[64] = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
    0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
    0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
    0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
    0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
    0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
    0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
    0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
    0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
    0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
    0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
    0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
    0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
    0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
    0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U
};

constexpr std::uint32_t rotateRight(
    std::uint32_t value,
    unsigned int count) noexcept {
    return (value >> count) | (value << (32U - count));
}

constexpr std::uint32_t choose(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t z) noexcept {
    return (x & y) ^ (~x & z);
}

constexpr std::uint32_t majority(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t z) noexcept {
    return (x & y) ^ (x & z) ^ (y & z);
}

void transform(Sha256Context& context, const std::uint8_t* block) noexcept {
    std::uint32_t schedule[64] = {};
    for (std::size_t i = 0; i < 16; ++i) {
        const std::size_t offset = i * 4;
        schedule[i] = (static_cast<std::uint32_t>(block[offset]) << 24) |
            (static_cast<std::uint32_t>(block[offset + 1]) << 16) |
            (static_cast<std::uint32_t>(block[offset + 2]) << 8) |
            static_cast<std::uint32_t>(block[offset + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
        const std::uint32_t value = schedule[i - 15];
        const std::uint32_t other = schedule[i - 2];
        const std::uint32_t small0 = rotateRight(value, 7) ^
            rotateRight(value, 18) ^ (value >> 3);
        const std::uint32_t small1 = rotateRight(other, 17) ^
            rotateRight(other, 19) ^ (other >> 10);
        schedule[i] = schedule[i - 16] + small0 + schedule[i - 7] + small1;
    }

    std::uint32_t a = context.state[0];
    std::uint32_t b = context.state[1];
    std::uint32_t c = context.state[2];
    std::uint32_t d = context.state[3];
    std::uint32_t e = context.state[4];
    std::uint32_t f = context.state[5];
    std::uint32_t g = context.state[6];
    std::uint32_t h = context.state[7];

    for (std::size_t i = 0; i < 64; ++i) {
        const std::uint32_t big1 = rotateRight(e, 6) ^
            rotateRight(e, 11) ^ rotateRight(e, 25);
        const std::uint32_t temp1 = h + big1 + choose(e, f, g) +
            roundConstants[i] + schedule[i];
        const std::uint32_t big0 = rotateRight(a, 2) ^
            rotateRight(a, 13) ^ rotateRight(a, 22);
        const std::uint32_t temp2 = big0 + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    context.state[0] += a;
    context.state[1] += b;
    context.state[2] += c;
    context.state[3] += d;
    context.state[4] += e;
    context.state[5] += f;
    context.state[6] += g;
    context.state[7] += h;
}

void initialize(Sha256Context& context) noexcept {
    context.state[0] = 0x6A09E667U;
    context.state[1] = 0xBB67AE85U;
    context.state[2] = 0x3C6EF372U;
    context.state[3] = 0xA54FF53AU;
    context.state[4] = 0x510E527FU;
    context.state[5] = 0x9B05688CU;
    context.state[6] = 0x1F83D9ABU;
    context.state[7] = 0x5BE0CD19U;
    context.totalBytes = 0;
    context.buffered = 0;
}

void update(Sha256Context& context, ByteView input) noexcept {
    context.totalBytes += static_cast<std::uint64_t>(input.size());
    std::size_t position = 0;
    while (position < input.size()) {
        const std::size_t available = sizeof(context.block) - context.buffered;
        const std::size_t remaining = input.size() - position;
        const std::size_t count = remaining < available ? remaining : available;
        for (std::size_t i = 0; i < count; ++i) {
            context.block[context.buffered + i] = input[position + i];
        }
        context.buffered += count;
        position += count;
        if (context.buffered == sizeof(context.block)) {
            transform(context, context.block);
            context.buffered = 0;
        }
    }
}

void finalize(
    Sha256Context& context,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept {
    context.block[context.buffered++] = 0x80U;
    if (context.buffered > 56) {
        while (context.buffered < sizeof(context.block)) {
            context.block[context.buffered++] = 0;
        }
        transform(context, context.block);
        context.buffered = 0;
    }
    while (context.buffered < 56) {
        context.block[context.buffered++] = 0;
    }

    const std::uint64_t bitCount = context.totalBytes * 8U;
    for (unsigned int i = 0; i < 8; ++i) {
        context.block[56 + i] = static_cast<std::uint8_t>(
            bitCount >> (56U - i * 8U));
    }
    transform(context, context.block);

    for (std::size_t i = 0; i < 8; ++i) {
        digest[i * 4] = static_cast<std::uint8_t>(context.state[i] >> 24);
        digest[i * 4 + 1] = static_cast<std::uint8_t>(context.state[i] >> 16);
        digest[i * 4 + 2] = static_cast<std::uint8_t>(context.state[i] >> 8);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(context.state[i]);
    }
}

} // namespace

Status sha256(
    ByteView input,
    std::uint8_t (&digest)[Sha256DigestSize]) noexcept {
    if (input.size() != 0 && input.data() == nullptr) {
        return Status::invalid_argument;
    }

    Sha256Context context{};
    initialize(context);
    update(context, input);

    std::uint8_t result[Sha256DigestSize] = {};
    finalize(context, result);
    for (std::size_t i = 0; i < Sha256DigestSize; ++i) {
        digest[i] = result[i];
    }
    return Status::ok;
}

} // namespace crypto
} // namespace util
} // namespace cms
