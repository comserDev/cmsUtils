#pragma once

#include <cstddef>

#include <cms/utf8.h>

namespace cms {
namespace detail {
namespace utf8 {

inline unsigned char byteAt(StringView input, std::size_t offset) noexcept {
    return static_cast<unsigned char>(input[offset]);
}

inline bool isContinuation(unsigned char byte) noexcept {
    return byte >= 0x80U && byte <= 0xBFU;
}

// 잘못된 sequence에서는 byte 하나만 소비해 다음 호출이 바로 다음 byte에서
// 다시 동기화할 수 있게 한다.
inline cms::utf8::DecodeResult invalid() noexcept {
    return {Status::invalid_utf8, static_cast<char32_t>(0xFFFDU), 1};
}

inline cms::utf8::DecodeResult decodeNext(
    StringView input,
    std::size_t offset) noexcept {
    if (offset >= input.size()) {
        return {Status::out_of_range, 0, 0};
    }

    const std::size_t remaining = input.size() - offset;
    const unsigned char first = byteAt(input, offset);

    if (first <= 0x7FU) {
        return {Status::ok, static_cast<char32_t>(first), 1};
    }

    if (first >= 0xC2U && first <= 0xDFU) {
        if (remaining < 2) {
            return invalid();
        }

        const unsigned char second = byteAt(input, offset + 1);
        if (!isContinuation(second)) {
            return invalid();
        }

        const char32_t codePoint =
            (static_cast<char32_t>(first & 0x1FU) << 6)
            | static_cast<char32_t>(second & 0x3FU);
        return {Status::ok, codePoint, 2};
    }

    if (first >= 0xE0U && first <= 0xEFU) {
        if (remaining < 3) {
            return invalid();
        }

        const unsigned char second = byteAt(input, offset + 1);
        const unsigned char third = byteAt(input, offset + 2);
        // E0의 overlong encoding과 ED의 surrogate 범위를 second byte에서 막는다.
        const bool secondValid =
            (first == 0xE0U && second >= 0xA0U && second <= 0xBFU)
            || (first >= 0xE1U && first <= 0xECU
                && isContinuation(second))
            || (first == 0xEDU && second >= 0x80U && second <= 0x9FU)
            || (first >= 0xEEU && first <= 0xEFU
                && isContinuation(second));

        if (!secondValid || !isContinuation(third)) {
            return invalid();
        }

        const char32_t codePoint =
            (static_cast<char32_t>(first & 0x0FU) << 12)
            | (static_cast<char32_t>(second & 0x3FU) << 6)
            | static_cast<char32_t>(third & 0x3FU);
        return {Status::ok, codePoint, 3};
    }

    if (first >= 0xF0U && first <= 0xF4U) {
        if (remaining < 4) {
            return invalid();
        }

        const unsigned char second = byteAt(input, offset + 1);
        const unsigned char third = byteAt(input, offset + 2);
        const unsigned char fourth = byteAt(input, offset + 3);
        // F0의 overlong encoding과 U+10FFFF를 넘는 F4 범위를 걸러낸다.
        const bool secondValid =
            (first == 0xF0U && second >= 0x90U && second <= 0xBFU)
            || (first >= 0xF1U && first <= 0xF3U
                && isContinuation(second))
            || (first == 0xF4U && second >= 0x80U && second <= 0x8FU);

        if (!secondValid
            || !isContinuation(third)
            || !isContinuation(fourth)) {
            return invalid();
        }

        const char32_t codePoint =
            (static_cast<char32_t>(first & 0x07U) << 18)
            | (static_cast<char32_t>(second & 0x3FU) << 12)
            | (static_cast<char32_t>(third & 0x3FU) << 6)
            | static_cast<char32_t>(fourth & 0x3FU);
        return {Status::ok, codePoint, 4};
    }

    return invalid();
}

} // namespace utf8
} // namespace detail
} // namespace cms
