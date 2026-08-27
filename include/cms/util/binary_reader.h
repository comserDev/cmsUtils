#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/status.h>

namespace cms {
namespace util {

class BinaryReader {
public:
    constexpr explicit BinaryReader(ByteView input) noexcept
        : input_(input), position_(0) {}

    constexpr std::size_t position() const noexcept { return position_; }
    constexpr std::size_t remaining() const noexcept {
        return input_.size() - position_;
    }
    constexpr bool empty() const noexcept { return remaining() == 0; }

    Status readUint8(std::uint8_t& value) noexcept {
        if (!has(1)) return Status::out_of_range;
        value = input_[position_];
        ++position_;
        return Status::ok;
    }

    Status readUint16BigEndian(std::uint16_t& value) noexcept {
        if (!has(2)) return Status::out_of_range;
        const std::size_t p = position_;
        value = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(input_[p]) << 8) |
            static_cast<std::uint16_t>(input_[p + 1]));
        position_ += 2;
        return Status::ok;
    }

    Status readUint32BigEndian(std::uint32_t& value) noexcept {
        if (!has(4)) return Status::out_of_range;
        const std::size_t p = position_;
        value = (static_cast<std::uint32_t>(input_[p]) << 24) |
            (static_cast<std::uint32_t>(input_[p + 1]) << 16) |
            (static_cast<std::uint32_t>(input_[p + 2]) << 8) |
            static_cast<std::uint32_t>(input_[p + 3]);
        position_ += 4;
        return Status::ok;
    }

    Status readUint64BigEndian(std::uint64_t& value) noexcept {
        if (!has(8)) return Status::out_of_range;
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < 8; ++i) {
            result = (result << 8) | input_[position_ + i];
        }
        value = result;
        position_ += 8;
        return Status::ok;
    }

    Status readBytes(std::size_t count, ByteView& value) noexcept {
        if (!has(count)) return Status::out_of_range;
        value = count == 0 ? ByteView() : ByteView(input_.data() + position_, count);
        position_ += count;
        return Status::ok;
    }

    Status skip(std::size_t count) noexcept {
        if (!has(count)) return Status::out_of_range;
        position_ += count;
        return Status::ok;
    }

private:
    constexpr bool has(std::size_t count) const noexcept {
        return count <= input_.size() - position_;
    }

    ByteView input_;
    std::size_t position_;
};

} // namespace util
} // namespace cms
