#pragma once

#include <cstddef>

#include <cms/status.h>
#include <cms/string_view.h>

namespace cms {

// Mutable, non-owning view of fixed-capacity NUL-terminated storage. The
// caller owns data and the shared size state and must keep both alive.
class StringBuffer {
public:
    constexpr StringBuffer() noexcept
        : data_(nullptr), capacity_(0), size_(nullptr) {}

    // capacity includes the terminating NUL byte. Invalid input is
    // canonicalized to the default invalid state.
    StringBuffer(
        char* data,
        std::size_t capacity,
        std::size_t& size) noexcept
        : data_(nullptr), capacity_(0), size_(nullptr) {
        if (data == nullptr || capacity == 0 || size >= capacity) {
            return;
        }

        if (data[size] != '\0') {
            return;
        }

        data_ = data;
        capacity_ = capacity;
        size_ = &size;
    }

    char* data() noexcept {
        return data_;
    }

    const char* data() const noexcept {
        return data_;
    }

    std::size_t size() const noexcept {
        return size_ != nullptr ? *size_ : 0;
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }

    std::size_t maxSize() const noexcept {
        return capacity_ > 0 ? capacity_ - 1 : 0;
    }

    std::size_t remaining() const noexcept {
        if (!valid()) {
            return 0;
        }

        return maxSize() - *size_;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    bool valid() const noexcept {
        if (!bound()) {
            return false;
        }

        return *size_ < capacity_ && data_[*size_] == '\0';
    }

    StringView view() const noexcept {
        if (!valid()) {
            return StringView();
        }

        return StringView(data_, *size_);
    }

    // clear() and commit() deliberately require only structural binding so
    // they can restore a buffer whose previous size or NUL was corrupted.
    Status clear() noexcept {
        if (!bound()) {
            return Status::invalid_argument;
        }

        *size_ = 0;
        data_[0] = '\0';
        return Status::ok;
    }

    // Publishes bytes already written in [data(), data() + newSize) and adds
    // the terminating NUL. A structurally bound buffer can be repaired even
    // when its previous size or NUL invariant was invalid.
    Status commit(std::size_t newSize) noexcept {
        if (!bound()) {
            return Status::invalid_argument;
        }

        if (newSize >= capacity_) {
            return Status::no_space;
        }

        *size_ = newSize;
        data_[newSize] = '\0';
        return Status::ok;
    }

private:
    bool bound() const noexcept {
        return data_ != nullptr && capacity_ > 0 && size_ != nullptr;
    }

    char* data_;
    std::size_t capacity_;
    std::size_t* size_;
};

} // namespace cms
