#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/status.h>

namespace cms {
namespace util {

// ByteView를 순차적으로 읽는 zero-copy reader다. 정수는 big-endian으로
// 해석하고, 읽기나 skip이 범위를 넘으면 cursor와 출력 값을 바꾸지 않는다.
class BinaryReader {
public:
    // input의 처음을 cursor로 설정한다. 입력 storage는 reader보다 오래 살아야 한다.
    constexpr explicit BinaryReader(ByteView input) noexcept
        : input_(input), position_(0) {}

    constexpr std::size_t position() const noexcept { return position_; }
    constexpr std::size_t remaining() const noexcept {
        return input_.size() - position_;
    }
    constexpr bool empty() const noexcept { return remaining() == 0; }

    // 한 byte를 읽고 성공할 때만 cursor와 value를 갱신한다.
    Status readUint8(std::uint8_t& value) noexcept {
        if (!has(1)) return Status::out_of_range;
        value = input_[position_];
        ++position_;
        return Status::ok;
    }

    // 두 byte를 big-endian 정수로 읽는다.
    Status readUint16BigEndian(std::uint16_t& value) noexcept {
        if (!has(2)) return Status::out_of_range;
        const std::size_t p = position_;
        value = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(input_[p]) << 8) |
            static_cast<std::uint16_t>(input_[p + 1]));
        position_ += 2;
        return Status::ok;
    }

    // 네 byte를 big-endian 정수로 읽는다.
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

    // 여덟 byte를 big-endian 정수로 읽는다.
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

    // input 내부의 다음 byte 구간을 zero-copy view로 반환한다.
    Status readBytes(std::size_t count, ByteView& value) noexcept {
        if (!has(count)) return Status::out_of_range;
        value = count == 0 ? ByteView() : ByteView(input_.data() + position_, count);
        position_ += count;
        return Status::ok;
    }

    // 지정한 byte 수만큼 cursor를 이동한다.
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
