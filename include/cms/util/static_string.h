#pragma once

#include <cstddef>
#include <cstring>

#include <cms/util/string_ops.h>

namespace cms {
namespace util {

// storage를 직접 소유하는 고정 capacity byte string이다. StorageBytes는
// terminating NUL 자리까지 포함하며 assign과 append는 UTF-8을 해석하지 않는다.
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

    // move는 payload를 destination의 자체 storage로 옮기고 source를 비운다.
    // 두 객체에 대해 기존에 만든 buffer alias는 각각 원래 storage에 남는다.
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

    // mutable raw access는 invariant 복구 경로가 있는 buffer()로만 제공한다.
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

    // 반환된 buffer는 이 객체의 storage와 size state를 빌리므로 객체보다 오래
    // 살 수 없다. move해도 이미 만들어진 view나 buffer를 다른 객체로 retarget하지
    // 않는다. raw editing 중에는 invariant가 일시적으로 깨질 수 있으므로 이
    // 문자열을 다시 관찰하기 전에 buffer.commit(newSize)이나 clear()로 복구한다.
    StringBuffer buffer() noexcept {
        return StringBuffer(data_, StorageBytes, size_);
    }

    constexpr void clear() noexcept {
        size_ = 0;
        data_[0] = '\0';
    }

    WriteResult assign(StringView value) noexcept {
        return string::copy(value, buffer());
    }

    WriteResult append(StringView value) noexcept {
        return string::append(value, buffer());
    }

    // Truncated variant는 공간이 부족하면 UTF-8 경계를 고려하지 않고 가능한
    // byte까지만 기록한다. UTF-8 단위 연산은 utf8 API를 사용해야 한다.
    WriteResult assignTruncated(StringView value) noexcept {
        return string::copyTruncated(value, buffer());
    }

    WriteResult appendTruncated(StringView value) noexcept {
        return string::appendTruncated(value, buffer());
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

} // namespace util
} // namespace cms
