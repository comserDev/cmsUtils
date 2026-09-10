#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace hash {

constexpr std::uint32_t Djb2InitialValue = 5381U;

// 32-bit modulo 연산을 고정한 DJB2 incremental hasher다. 입력을 byte 단위로
// 처리하며 case 변환이나 다른 문자열 정규화는 수행하지 않는다.
class Djb2 {
public:
    // DJB2 표준 초기값 5381로 시작한다.
    constexpr Djb2() noexcept
        : value_(Djb2InitialValue) {}

    // 초기값으로 되돌린다.
    constexpr void reset() noexcept {
        value_ = Djb2InitialValue;
    }

    // 한 byte를 hash에 반영한다.
    constexpr void updateByte(std::uint8_t value) noexcept {
        const std::uint32_t multiplied = static_cast<std::uint32_t>(
            value_ * static_cast<std::uint32_t>(33U));
        value_ = static_cast<std::uint32_t>(
            multiplied + static_cast<std::uint32_t>(value));
    }

    // binary input의 모든 byte를 순서대로 반영한다.
    constexpr void update(ByteView input) noexcept {
        for (std::size_t index = 0; index < input.size(); ++index) {
            updateByte(input[index]);
        }
    }

    // StringView의 각 byte를 unsigned byte로 해석해 반영한다.
    constexpr void update(StringView input) noexcept {
        for (std::size_t index = 0; index < input.size(); ++index) {
            updateByte(static_cast<std::uint8_t>(
                static_cast<unsigned char>(input[index])));
        }
    }

    // 현재 32-bit hash 값을 반환한다.
    constexpr std::uint32_t value() const noexcept {
        return value_;
    }

private:
    std::uint32_t value_;
};

// binary input의 DJB2 값을 한 번에 계산한다.
constexpr std::uint32_t djb2(ByteView input) noexcept {
    Djb2 hash;
    hash.update(input);
    return hash.value();
}

// 문자열의 raw byte를 기준으로 DJB2 값을 한 번에 계산한다.
constexpr std::uint32_t djb2(StringView input) noexcept {
    Djb2 hash;
    hash.update(input);
    return hash.value();
}

} // namespace hash
} // namespace util
} // namespace cms
