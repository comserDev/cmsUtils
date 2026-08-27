#pragma once

#include <cstddef>
#include <cstdint>

#include <cms/util/byte_view.h>
#include <cms/util/status.h>

namespace cms {
namespace util {

// Caller-owned storage와 size state를 공유하는 fixed-capacity binary buffer다.
class ByteBuffer {
public:
    constexpr ByteBuffer() noexcept
        : data_(nullptr), capacity_(0), size_(nullptr) {}

    ByteBuffer(
        std::uint8_t* data,
        std::size_t capacity,
        std::size_t& size) noexcept
        : data_(nullptr), capacity_(0), size_(nullptr) {
        if (size > capacity || (capacity > 0 && data == nullptr)) {
            return;
        }
        data_ = data;
        capacity_ = capacity;
        size_ = &size;
    }

    std::uint8_t* data() noexcept { return data_; }
    const std::uint8_t* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return valid() ? *size_ : 0; }
    std::size_t capacity() const noexcept { return valid() ? capacity_ : 0; }
    std::size_t remaining() const noexcept {
        return valid() ? capacity_ - *size_ : 0;
    }
    bool empty() const noexcept { return size() == 0; }
    bool valid() const noexcept {
        return size_ != nullptr && *size_ <= capacity_ &&
            (capacity_ == 0 || data_ != nullptr);
    }
    ByteView view() const noexcept {
        return valid() ? ByteView(data_, *size_) : ByteView();
    }

    Status clear() noexcept {
        if (!valid()) {
            return Status::invalid_argument;
        }
        *size_ = 0;
        return Status::ok;
    }

    // Raw storage에 기록한 [0, newSize) 구간을 publish한다.
    Status commit(std::size_t newSize) noexcept {
        if (!valid()) {
            return Status::invalid_argument;
        }
        if (newSize > capacity_) {
            return Status::no_space;
        }
        *size_ = newSize;
        return Status::ok;
    }

private:
    std::uint8_t* data_;
    std::size_t capacity_;
    std::size_t* size_;
};

} // namespace util
} // namespace cms
