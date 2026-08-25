#pragma once

#include <cstddef>

namespace cms {

// 읽기 전용 non-owning byte view다. NUL 종료를 보장하지 않으며 문자열
// literal의 terminating NUL은 view에 포함하지 않는다.
class StringView {
public:
    constexpr StringView() noexcept
        : data_(nullptr), size_(0) {}

    // nullptr은 크기와 관계없이 빈 view로 canonicalize한다. operator[]의
    // precondition은 index < size()이며 의도적으로 bounds check를 하지 않는다.
    constexpr StringView(const char* data, std::size_t size) noexcept
        : data_(data), size_(data != nullptr ? size : 0) {}

    // 배열 convenience constructor는 N 범위 안에서 첫 NUL까지만 취하는
    // bounded C-string semantics를 사용한다. NUL이 없으면 N byte 전체를 본다.
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

    // byte offset 기준 subview를 만든다. count가 남은 길이보다 크면 끝까지
    // clamp하며 offset이 범위를 벗어나면 빈 view를 반환한다.
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
