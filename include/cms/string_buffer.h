#pragma once

#include <cstddef>

#include <cms/status.h>
#include <cms/string_view.h>

namespace cms {

// 고정 capacity의 NUL-terminated storage를 가리키는 mutable non-owning view다.
// caller가 data와 공유 size state를 소유하며 둘의 lifetime을 보장해야 한다.
// StringBuffer를 복사하거나 이동해도 같은 storage와 size state를 alias한다.
class StringBuffer {
public:
    constexpr StringBuffer() noexcept
        : data_(nullptr), capacity_(0), size_(nullptr) {}

    // capacity는 terminating NUL 자리까지 포함한다. 생성 시 size와 NUL
    // invariant가 맞지 않으면 default와 같은 unbound 상태로 canonicalize한다.
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

    // data()로 raw editing한 뒤에는 commit()이나 clear()로 size와 NUL
    // invariant를 복구해야 한다.
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

    // bound 상태이고 size < capacity이며 data[size]가 NUL일 때만 유효하다.
    bool valid() const noexcept {
        if (!bound()) {
            return false;
        }

        return *size_ < capacity_ && data_[*size_] == '\0';
    }

    // 현재 pointer와 size를 snapshot한다. 이후 size 변경은 반영되지 않지만
    // underlying storage는 계속 alias한다.
    StringView view() const noexcept {
        if (!valid()) {
            return StringView();
        }

        return StringView(data_, *size_);
    }

    // clear()와 commit()은 손상된 size나 NUL 상태도 복구할 수 있도록
    // 의도적으로 structural binding만 요구한다.
    Status clear() noexcept {
        if (!bound()) {
            return Status::invalid_argument;
        }

        *size_ = 0;
        data_[0] = '\0';
        return Status::ok;
    }

    // caller가 [data(), data() + newSize)에 미리 기록한 byte를 publish하고
    // terminating NUL을 붙인다. 기존 size나 NUL invariant가 깨졌더라도
    // structurally bound 상태라면 새로운 유효 상태로 복구할 수 있다.
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
