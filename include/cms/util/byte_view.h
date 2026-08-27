#pragma once

#include <cstddef>
#include <cstdint>

namespace cms {
namespace util {

// Binary data를 소유하지 않는 read-only view다. Embedded NUL도 일반 byte로 취급한다.
class ByteView {
public:
    constexpr ByteView() noexcept
        : data_(nullptr), size_(0) {}

    constexpr ByteView(const std::uint8_t* data, std::size_t size) noexcept
        : data_(data), size_(data != nullptr ? size : 0) {}

    template<std::size_t N>
    constexpr ByteView(const std::uint8_t (&data)[N]) noexcept
        : data_(data), size_(N) {}

    constexpr const std::uint8_t* data() const noexcept { return data_; }
    constexpr std::size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr std::uint8_t operator[](std::size_t index) const noexcept {
        return data_[index];
    }

    constexpr ByteView subview(
        std::size_t offset,
        std::size_t count) const noexcept {
        if (offset > size_ || data_ == nullptr) {
            return ByteView();
        }

        const std::size_t available = size_ - offset;
        return ByteView(data_ + offset, count < available ? count : available);
    }

private:
    const std::uint8_t* data_;
    std::size_t size_;
};

} // namespace util
} // namespace cms
