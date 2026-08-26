#include <cms/util/format.h>

#include <cstddef>

namespace cms {
namespace util {
namespace format {

namespace {

// uint64_t decimal은 최대 20자리다. 여분 한 byte는 가능한 minus sign 자리이며
// 이 scratch buffer에는 terminating NUL을 저장하지 않는다.
constexpr std::size_t scratchSize = 21;

bool supportedBase(unsigned int base) noexcept {
    return base == 10 || base == 16;
}

char digitCharacter(unsigned int digit, bool uppercase) noexcept {
    if (digit < 10) {
        return static_cast<char>('0' + digit);
    }

    const char firstLetter = uppercase ? 'A' : 'a';
    return static_cast<char>(firstLetter + (digit - 10));
}

struct EncodedInteger {
    char bytes[scratchSize];
    std::size_t offset;
};

EncodedInteger encode(
    std::uint64_t magnitude,
    bool negative,
    unsigned int base,
    bool uppercase) noexcept {
    EncodedInteger result{};
    result.offset = scratchSize;

    do {
        const unsigned int digit =
            static_cast<unsigned int>(magnitude % base);
        --result.offset;
        result.bytes[result.offset] = digitCharacter(digit, uppercase);
        magnitude /= base;
    } while (magnitude != 0);

    if (negative) {
        --result.offset;
        result.bytes[result.offset] = '-';
    }

    return result;
}

WriteResult writeEncoded(
    const EncodedInteger& encoded,
    StringBuffer output,
    bool append) noexcept {
    const std::size_t required = scratchSize - encoded.offset;
    const std::size_t available = append ? output.remaining() : output.maxSize();
    if (required > available) {
        return {Status::no_space, 0, required};
    }

    // capacity를 전부 확인한 뒤에만 destination에 쓰기 시작한다.
    const std::size_t oldSize = append ? output.size() : 0;
    for (std::size_t index = 0; index < required; ++index) {
        output.data()[oldSize + index] = encoded.bytes[encoded.offset + index];
    }

    const Status status = output.commit(oldSize + required);
    if (status != Status::ok) {
        return {status, 0, 0};
    }
    return {Status::ok, required, required};
}

WriteResult formatMagnitude(
    std::uint64_t magnitude,
    bool negative,
    StringBuffer output,
    unsigned int base,
    bool uppercase,
    bool append) noexcept {
    if (!output.valid() || !supportedBase(base)) {
        return {Status::invalid_argument, 0, 0};
    }

    const EncodedInteger encoded = encode(magnitude, negative, base, uppercase);
    return writeEncoded(encoded, output, append);
}

std::uint64_t magnitudeOf(std::int64_t value) noexcept {
    if (value >= 0) {
        return static_cast<std::uint64_t>(value);
    }

    // uint64_t 변환과 뺄셈은 INT64_MIN을 포함해 modulo 2^64로 정의된다.
    return std::uint64_t{0} - static_cast<std::uint64_t>(value);
}

} // namespace

WriteResult unsignedInteger(
    std::uint64_t value,
    StringBuffer output,
    unsigned int base,
    bool uppercase) noexcept {
    return formatMagnitude(value, false, output, base, uppercase, false);
}

WriteResult signedInteger(
    std::int64_t value,
    StringBuffer output,
    unsigned int base,
    bool uppercase) noexcept {
    return formatMagnitude(
        magnitudeOf(value),
        value < 0,
        output,
        base,
        uppercase,
        false);
}

WriteResult appendUnsignedInteger(
    std::uint64_t value,
    StringBuffer output,
    unsigned int base,
    bool uppercase) noexcept {
    return formatMagnitude(value, false, output, base, uppercase, true);
}

WriteResult appendSignedInteger(
    std::int64_t value,
    StringBuffer output,
    unsigned int base,
    bool uppercase) noexcept {
    return formatMagnitude(
        magnitudeOf(value),
        value < 0,
        output,
        base,
        uppercase,
        true);
}

} // namespace format
} // namespace util
} // namespace cms
