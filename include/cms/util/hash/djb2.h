#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace hash {

constexpr std::uint32_t Djb2InitialValue = 5381U;

class Djb2 {
public:
    constexpr Djb2() noexcept
        : value_(Djb2InitialValue) {}

    constexpr void reset() noexcept {
        value_ = Djb2InitialValue;
    }

    constexpr void updateByte(std::uint8_t value) noexcept {
        const std::uint32_t multiplied = static_cast<std::uint32_t>(
            value_ * static_cast<std::uint32_t>(33U));
        value_ = static_cast<std::uint32_t>(
            multiplied + static_cast<std::uint32_t>(value));
    }

    constexpr void update(ByteView input) noexcept {
        for (std::size_t index = 0; index < input.size(); ++index) {
            updateByte(input[index]);
        }
    }

    constexpr void update(StringView input) noexcept {
        for (std::size_t index = 0; index < input.size(); ++index) {
            updateByte(static_cast<std::uint8_t>(
                static_cast<unsigned char>(input[index])));
        }
    }

    constexpr std::uint32_t value() const noexcept {
        return value_;
    }

private:
    std::uint32_t value_;
};

constexpr std::uint32_t djb2(ByteView input) noexcept {
    Djb2 hash;
    hash.update(input);
    return hash.value();
}

constexpr std::uint32_t djb2(StringView input) noexcept {
    Djb2 hash;
    hash.update(input);
    return hash.value();
}

} // namespace hash
} // namespace util
} // namespace cms
