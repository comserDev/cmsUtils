#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <cms/util/byte_buffer.h>

namespace cms {
namespace util {

class BinaryWriter {
public:
    explicit BinaryWriter(ByteBuffer output) noexcept : output_(output) {}

    std::size_t position() const noexcept { return output_.size(); }
    std::size_t remaining() const noexcept { return output_.remaining(); }
    bool valid() const noexcept { return output_.valid(); }

    Status writeUint8(std::uint8_t value) noexcept {
        return writeInteger(value, 1);
    }
    Status writeUint16BigEndian(std::uint16_t value) noexcept {
        return writeInteger(value, 2);
    }
    Status writeUint32BigEndian(std::uint32_t value) noexcept {
        return writeInteger(value, 4);
    }
    Status writeUint64BigEndian(std::uint64_t value) noexcept {
        return writeInteger(value, 8);
    }

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
