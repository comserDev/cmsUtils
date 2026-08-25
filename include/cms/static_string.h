#pragma once

#include <cstddef>
#include <cstring>

#include <cms/status.h>
#include <cms/string_buffer.h>
#include <cms/string_view.h>

namespace cms {

// Owning fixed-capacity byte string. StorageBytes includes the terminating
// NUL byte; assign and append do not interpret UTF-8.
template<std::size_t StorageBytes>
class StaticString {
    static_assert(
        StorageBytes > 0,
        "StaticString requires at least one storage byte");

public:
    constexpr StaticString() noexcept
        : data_{}, size_(0) {}

    StaticString(const StaticString&) noexcept = default;
    StaticString& operator=(const StaticString&) noexcept = default;

    StaticString(StaticString&& other) noexcept
        : data_{}, size_(0) {
        moveFrom(other);
    }

    StaticString& operator=(StaticString&& other) noexcept {
        if (this != &other) {
            moveFrom(other);
        }
        return *this;
    }

    constexpr const char* cStr() const noexcept {
        return data_;
    }

    // Mutable raw access is intentionally available only through buffer().
    constexpr const char* data() const noexcept {
        return data_;
    }

    constexpr std::size_t size() const noexcept {
        return size_;
    }

    constexpr std::size_t capacity() const noexcept {
        return StorageBytes;
    }

    constexpr std::size_t maxSize() const noexcept {
        return StorageBytes - 1;
    }

    constexpr std::size_t remaining() const noexcept {
        return maxSize() - size_;
    }

    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    constexpr StringView view() const noexcept {
        return StringView(data_, size_);
    }

    // The returned buffer borrows this object's storage and size state and
    // must not outlive this object; move does not retarget existing views.
    // Raw edits can temporarily suspend this string's invariant. Call
    // buffer.commit(newSize) or buffer.clear() before observing this string.
    StringBuffer buffer() noexcept {
        return StringBuffer(data_, StorageBytes, size_);
    }

    constexpr void clear() noexcept {
        size_ = 0;
        data_[0] = '\0';
    }

    WriteResult assign(StringView value) noexcept {
        if (value.size() > maxSize()) {
            return {Status::no_space, 0, value.size()};
        }

        copyBytes(data_, value.data(), value.size());
        setSize(value.size());
        return {Status::ok, value.size(), value.size()};
    }

    WriteResult append(StringView value) noexcept {
        if (value.size() > remaining()) {
            return {Status::no_space, 0, value.size()};
        }

        const std::size_t oldSize = size_;
        copyBytes(data_ + oldSize, value.data(), value.size());
        setSize(oldSize + value.size());
        return {Status::ok, value.size(), value.size()};
    }

    WriteResult assignTruncated(StringView value) noexcept {
        const std::size_t copySize =
            value.size() < maxSize() ? value.size() : maxSize();
        copyBytes(data_, value.data(), copySize);
        setSize(copySize);

        const Status status =
            copySize == value.size() ? Status::ok : Status::no_space;
        return {status, copySize, value.size()};
    }

    WriteResult appendTruncated(StringView value) noexcept {
        const std::size_t available = remaining();
        const std::size_t copySize =
            value.size() < available ? value.size() : available;
        const std::size_t oldSize = size_;
        copyBytes(data_ + oldSize, value.data(), copySize);
        setSize(oldSize + copySize);

        const Status status =
            copySize == value.size() ? Status::ok : Status::no_space;
        return {status, copySize, value.size()};
    }

private:
    static void copyBytes(
        char* destination,
        const char* source,
        std::size_t count) noexcept {
        if (count > 0) {
            std::memmove(destination, source, count);
        }
    }

    constexpr void setSize(std::size_t newSize) noexcept {
        size_ = newSize;
        data_[newSize] = '\0';
    }

    void moveFrom(StaticString& other) noexcept {
        copyBytes(data_, other.data_, other.size_);
        setSize(other.size_);
        other.clear();
    }

    char data_[StorageBytes];
    std::size_t size_;
};

} // namespace cms
