#include <cms/parse.h>

#include <cstddef>
#include <limits>

namespace cms {
namespace parse {

namespace {

bool supportedBase(unsigned int base) noexcept {
    return base == 10 || base == 16;
}

bool digitValue(char byte, unsigned int base, unsigned int& digit) noexcept {
    const unsigned char value = static_cast<unsigned char>(byte);
    if (value >= static_cast<unsigned char>('0')
        && value <= static_cast<unsigned char>('9')) {
        digit = static_cast<unsigned int>(value - static_cast<unsigned char>('0'));
    } else if (value >= static_cast<unsigned char>('a')
        && value <= static_cast<unsigned char>('f')) {
        digit = 10U
            + static_cast<unsigned int>(value - static_cast<unsigned char>('a'));
    } else if (value >= static_cast<unsigned char>('A')
        && value <= static_cast<unsigned char>('F')) {
        digit = 10U
            + static_cast<unsigned int>(value - static_cast<unsigned char>('A'));
    } else {
        return false;
    }

    return digit < base;
}

std::size_t skipHexPrefix(
    StringView input,
    std::size_t offset,
    unsigned int base) noexcept {
    if (base != 16 || input.size() - offset < 2 || input[offset] != '0') {
        return offset;
    }

    // prefix 뒤 digit 존재 여부는 공통 magnitude parser가 검사한다.
    const char marker = input[offset + 1];
    return marker == 'x' || marker == 'X' ? offset + 2 : offset;
}

ParseResult<std::uint64_t> parseMagnitude(
    StringView input,
    std::size_t digitOffset,
    unsigned int base,
    std::uint64_t limit) noexcept {
    std::uint64_t value = 0;
    std::size_t offset = digitOffset;
    bool hasDigit = false;

    while (offset < input.size()) {
        unsigned int digit = 0;
        if (!digitValue(input[offset], base, digit)) {
            break;
        }

        hasDigit = true;
        const std::uint64_t unsignedDigit = static_cast<std::uint64_t>(digit);
        // offending digit을 연산하기 전에 검사해 unsigned wraparound를 막는다.
        if (value > (limit - unsignedDigit) / base) {
            return {Status::out_of_range, 0, offset};
        }

        value = value * base + unsignedDigit;
        ++offset;
    }

    if (!hasDigit) {
        return {Status::invalid_argument, 0, 0};
    }
    return {Status::ok, value, offset};
}

} // namespace

ParseResult<std::uint64_t> unsignedInteger(
    StringView input,
    unsigned int base) noexcept {
    if (!supportedBase(base) || input.empty()) {
        return {Status::invalid_argument, 0, 0};
    }
    if (input[0] == '+' || input[0] == '-') {
        return {Status::invalid_argument, 0, 0};
    }

    const std::size_t digitOffset = skipHexPrefix(input, 0, base);
    return parseMagnitude(
        input,
        digitOffset,
        base,
        (std::numeric_limits<std::uint64_t>::max)());
}

ParseResult<std::int64_t> signedInteger(
    StringView input,
    unsigned int base) noexcept {
    if (!supportedBase(base) || input.empty()) {
        return {Status::invalid_argument, 0, 0};
    }

    std::size_t offset = 0;
    bool negative = false;
    if (input[0] == '+' || input[0] == '-') {
        negative = input[0] == '-';
        offset = 1;
    }

    const std::size_t digitOffset = skipHexPrefix(input, offset, base);
    const std::uint64_t positiveLimit =
        static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
    const std::uint64_t limit = negative ? positiveLimit + 1U : positiveLimit;
    const ParseResult<std::uint64_t> magnitude =
        parseMagnitude(input, digitOffset, base, limit);
    if (magnitude.status != Status::ok) {
        return {magnitude.status, 0, magnitude.consumed};
    }

    if (!negative) {
        return {
            Status::ok,
            static_cast<std::int64_t>(magnitude.value),
            magnitude.consumed};
    }
    // INT64_MIN magnitude는 int64_t 양수로 표현할 수 없으므로 직접 반환한다.
    if (magnitude.value == positiveLimit + 1U) {
        return {
            Status::ok,
            (std::numeric_limits<std::int64_t>::min)(),
            magnitude.consumed};
    }
    return {
        Status::ok,
        -static_cast<std::int64_t>(magnitude.value),
        magnitude.consumed};
}

} // namespace parse
} // namespace cms
