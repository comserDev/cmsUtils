#pragma once

#include <cstddef>

namespace cms {

// Read-only, non-owning byte view. NUL termination is not guaranteed and the
// terminating NUL of a string literal is not part of the view.
class StringView {
public:
    constexpr StringView() noexcept
        : data_(nullptr), size_(0) {}

    // A null pointer is canonicalized to an empty view. operator[] requires
    // index < size(); it intentionally performs no bounds checking.
    constexpr StringView(const char* data, std::size_t size) noexcept
        : data_(data), size_(data != nullptr ? size : 0) {}

    // Convenience construction from an array uses bounded C-string
    // semantics. An array without a NUL byte consumes all N bytes.
    template<std::size_t N>
    constexpr StringView(const char (&array)[N]) noexcept
        : data_(array), size_(0) {
        while (size_ < N && array[size_] != '\0') {
            ++size_;
        }
    }

    constexpr const char* data() const noexcept {
        return data_;
    }

    constexpr std::size_t size() const noexcept {
        return size_;
    }

    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    constexpr char operator[](std::size_t index) const noexcept {
        return data_[index];
    }

    constexpr StringView substr(
        std::size_t offset,
        std::size_t count) const noexcept {
        if (offset > size_ || data_ == nullptr) {
            return StringView();
        }

        const std::size_t available = size_ - offset;
        const std::size_t resultSize = count < available ? count : available;
        return StringView(data_ + offset, resultSize);
    }

private:
    const char* data_;
    std::size_t size_;
};

} // namespace cms
