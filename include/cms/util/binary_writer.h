#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <cms/util/byte_buffer.h>

namespace cms {
namespace util {

// caller가 소유한 ByteBuffer에 정수와 raw byte를 순서대로 기록하는 writer다.
// 정수는 big-endian으로 기록하며 공간이 부족하거나 buffer가 invalid하면
// 해당 연산은 output size와 payload를 바꾸지 않는다.
class BinaryWriter {
public:
    // output의 현재 size부터 기록을 시작한다. writer는 storage를 소유하지 않는다.
    explicit BinaryWriter(ByteBuffer output) noexcept : output_(output) {}

    std::size_t position() const noexcept { return output_.size(); }
    std::size_t remaining() const noexcept { return output_.remaining(); }
    bool valid() const noexcept { return output_.valid(); }

    // 1 byte 정수를 기록한다.
    Status writeUint8(std::uint8_t value) noexcept {
        return writeInteger(value, 1);
    }
    // 2 byte big-endian 정수를 기록한다.
    Status writeUint16BigEndian(std::uint16_t value) noexcept {
        return writeInteger(value, 2);
    }
    // 4 byte big-endian 정수를 기록한다.
    Status writeUint32BigEndian(std::uint32_t value) noexcept {
        return writeInteger(value, 4);
    }
    // 8 byte big-endian 정수를 기록한다.
    Status writeUint64BigEndian(std::uint64_t value) noexcept {
        return writeInteger(value, 8);
    }

    // raw byte를 기록한다. input과 output storage가 겹쳐도 안전하게 처리한다.
    Status writeBytes(ByteView value) noexcept {
        if (!output_.valid()) return Status::invalid_argument;
        if (value.size() > output_.remaining()) return Status::no_space;
        if (value.size() > 0 && value.data() == nullptr) {
            return Status::invalid_argument;
        }
        const std::size_t position = output_.size();
        if (value.size() > 0) {
            std::memmove(output_.data() + position, value.data(), value.size());
        }
        return output_.commit(position + value.size());
    }

private:
    template<class T>
    Status writeInteger(T value, std::size_t width) noexcept {
        if (!output_.valid()) return Status::invalid_argument;
        if (width > output_.remaining()) return Status::no_space;
        const std::size_t position = output_.size();
        for (std::size_t i = 0; i < width; ++i) {
            const std::size_t shift = (width - i - 1) * 8;
            output_.data()[position + i] = static_cast<std::uint8_t>(value >> shift);
        }
        return output_.commit(position + width);
    }

    ByteBuffer output_;
};

} // namespace util
} // namespace cms
