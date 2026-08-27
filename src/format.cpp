#include <cms/util/format.h>

#include <cmath>
#include <cstddef>
#include <limits>

namespace cms {
namespace util {
namespace format {

namespace {

// uint64_t decimal은 최대 20자리다. 여분 한 byte는 가능한 minus sign 자리이며
// 이 scratch buffer에는 terminating NUL을 저장하지 않는다.
constexpr std::size_t scratchSize = 21;
constexpr std::size_t floatingScratchSize = 31;
constexpr unsigned int maximumDecimalPlaces = 9;
constexpr double uint64ExclusiveUpperBound = 18446744073709551616.0;

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

struct EncodedFloatingPoint {
    char bytes[floatingScratchSize];
    std::size_t size;
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

std::uint64_t decimalScale(unsigned int decimalPlaces) noexcept {
    std::uint64_t scale = 1;
    for (unsigned int index = 0; index < decimalPlaces; ++index) {
        scale *= 10;
    }
    return scale;
}

bool encodeFloatingPoint(
    double value,
    unsigned int decimalPlaces,
    EncodedFloatingPoint& encoded) noexcept {
    const bool negative = std::signbit(value);
    const double magnitude = negative ? -value : value;
    if (magnitude >= uint64ExclusiveUpperBound) {
        return false;
    }

    double integerValue = 0.0;
    const double fraction = std::modf(magnitude, &integerValue);
    std::uint64_t integerMagnitude =
        static_cast<std::uint64_t>(integerValue);
    const std::uint64_t scale = decimalScale(decimalPlaces);
    const double doubleScale = static_cast<double>(scale);
    const double scaled = fraction * doubleScale;
    double scaledInteger = 0.0;
    const double remainder = std::modf(scaled, &scaledInteger);
    std::uint64_t fractionalMagnitude =
        static_cast<std::uint64_t>(scaledInteger);
    bool roundUp = remainder > 0.5;
    if (remainder == 0.5) {
        // 곱셈 결과가 0.5로 반올림된 경우 FMA residual로 실제 midpoint 방향을 판정한다.
        const double residual = std::fma(fraction, doubleScale, -scaled);
        roundUp = residual >= 0.0;
    }
    if (roundUp) {
        ++fractionalMagnitude;
    }
    if (fractionalMagnitude == scale) {
        if (integerMagnitude
            == (std::numeric_limits<std::uint64_t>::max)()) {
            return false;
        }
        ++integerMagnitude;
        fractionalMagnitude = 0;
    }

    const EncodedInteger integer = encode(
        integerMagnitude,
        negative,
        10,
        false);
    encoded.size = scratchSize - integer.offset;
    for (std::size_t index = 0; index < encoded.size; ++index) {
        encoded.bytes[index] = integer.bytes[integer.offset + index];
    }

    if (decimalPlaces == 0) {
        return true;
    }

    encoded.bytes[encoded.size++] = '.';
    std::size_t digitPosition = encoded.size + decimalPlaces;
    encoded.size = digitPosition;
    while (digitPosition > encoded.size - decimalPlaces) {
        --digitPosition;
        encoded.bytes[digitPosition] = static_cast<char>(
            '0' + (fractionalMagnitude % 10));
        fractionalMagnitude /= 10;
    }
    return true;
}

WriteResult formatFloatingPoint(
    double value,
    StringBuffer output,
    unsigned int decimalPlaces,
    bool append) noexcept {
    if (!output.valid() || decimalPlaces > maximumDecimalPlaces
        || !std::isfinite(value)) {
        return {Status::invalid_argument, 0, 0};
    }

    EncodedFloatingPoint encoded{};
    if (!encodeFloatingPoint(value, decimalPlaces, encoded)) {
        return {Status::out_of_range, 0, 0};
    }

    const std::size_t available = append ? output.remaining() : output.maxSize();
    if (encoded.size > available) {
        return {Status::no_space, 0, encoded.size};
    }

    const std::size_t oldSize = append ? output.size() : 0;
    for (std::size_t index = 0; index < encoded.size; ++index) {
        output.data()[oldSize + index] = encoded.bytes[index];
    }
    const Status status = output.commit(oldSize + encoded.size);
    if (status != Status::ok) {
        return {status, 0, 0};
    }
    return {Status::ok, encoded.size, encoded.size};
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

WriteResult floatingPoint(
    double value,
    StringBuffer output,
    unsigned int decimalPlaces) noexcept {
    return formatFloatingPoint(value, output, decimalPlaces, false);
}

WriteResult appendFloatingPoint(
    double value,
    StringBuffer output,
    unsigned int decimalPlaces) noexcept {
    return formatFloatingPoint(value, output, decimalPlaces, true);
}

} // namespace format
} // namespace util
} // namespace cms
