#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_buffer.h>

namespace cms {
namespace util {

template<std::size_t Capacity>
class StaticByteBuffer {
    static_assert(Capacity > 0, "StaticByteBuffer capacity must be positive");

public:
    constexpr StaticByteBuffer() noexcept : data_{}, size_(0) {}

    std::uint8_t* data() noexcept { return data_; }
    constexpr const std::uint8_t* data() const noexcept { return data_; }
    constexpr std::size_t size() const noexcept { return size_; }
    constexpr std::size_t capacity() const noexcept { return Capacity; }
    constexpr std::size_t remaining() const noexcept { return Capacity - size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr ByteView view() const noexcept { return ByteView(data_, size_); }
    ByteBuffer buffer() noexcept { return ByteBuffer(data_, Capacity, size_); }
    constexpr void clear() noexcept { size_ = 0; }

private:
    std::uint8_t data_[Capacity];
    std::size_t size_;
};

} // namespace util
} // namespace cms
