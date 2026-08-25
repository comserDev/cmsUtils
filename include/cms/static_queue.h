#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include <cms/detail/small_index.h>
#include <cms/status.h>

namespace cms {

// live element만 생성하는 fixed-capacity FIFO queue다. Capacity는 0보다 커야
// 하고 T의 destructor는 noexcept여야 한다. Heap allocation과 synchronization을
// 제공하지 않으며 동시 접근 제어는 caller의 책임이다.
template<class T, std::size_t Capacity>
class StaticQueue {
    static_assert(Capacity > 0, "StaticQueue capacity must be greater than zero");
    static_assert(
        std::is_nothrow_destructible<T>::value,
        "StaticQueue requires a nothrow-destructible element type");

    struct Slot {
        alignas(T) unsigned char bytes[sizeof(T)];
    };

    using Index = detail::small_index_t<Capacity>;

public:
    StaticQueue() noexcept
        : head_(0), size_(0) {}

    ~StaticQueue() noexcept {
        clear();
    }

    StaticQueue(const StaticQueue&) = delete;
    StaticQueue& operator=(const StaticQueue&) = delete;
    StaticQueue(StaticQueue&&) = delete;
    StaticQueue& operator=(StaticQueue&&) = delete;

    std::size_t size() const noexcept {
        return static_cast<std::size_t>(size_);
    }

    constexpr std::size_t capacity() const noexcept {
        return Capacity;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    bool full() const noexcept {
        return size() == Capacity;
    }

    // 반환 pointer는 해당 element를 pop/clear하거나 queue가 파괴될 때까지
    // 유효하다. 다른 element를 push해도 기존 front는 이동하지 않는다.
    T* front() noexcept {
        return empty() ? nullptr : pointerAt(headIndex());
    }

    const T* front() const noexcept {
        return empty() ? nullptr : pointerAt(headIndex());
    }

    Status push(const T& value)
        noexcept(std::is_nothrow_copy_constructible<T>::value) {
        return emplace(value);
    }

    Status push(T&& value)
        noexcept(std::is_nothrow_move_constructible<T>::value) {
        return emplace(std::move(value));
    }

    template<class... Args>
    Status emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible<T, Args...>::value) {
        // full이면 constructor를 호출하지 않고 기존 queue를 그대로 둔다.
        if (full()) {
            return Status::no_space;
        }

        const std::size_t tail = tailIndex();
        ::new (static_cast<void*>(storage_[tail].bytes))
            T(std::forward<Args>(args)...);

        // construction이 끝난 뒤에만 live element 수를 공개한다.
        size_ = static_cast<Index>(size() + 1);
        return Status::ok;
    }

    // front를 즉시 destroy한다. 빈 queue에서는 out_of_range만 반환한다.
    Status pop() noexcept {
        if (empty()) {
            return Status::out_of_range;
        }

        pointerAt(headIndex())->~T();
        advanceHead();
        size_ = static_cast<Index>(size() - 1);
        return Status::ok;
    }

    // FIFO 순서로 모든 live element를 destroy하고 metadata를 초기 상태로 돌린다.
    void clear() noexcept {
        while (!empty()) {
            pointerAt(headIndex())->~T();
            advanceHead();
            size_ = static_cast<Index>(size() - 1);
        }

        head_ = 0;
    }

private:
    std::size_t headIndex() const noexcept {
        return static_cast<std::size_t>(head_);
    }

    // head + size를 먼저 계산하지 않아 std::size_t wraparound를 피한다.
    std::size_t tailIndex() const noexcept {
        const std::size_t head = headIndex();
        const std::size_t currentSize = size();
        const std::size_t spaceToEnd = Capacity - head;
        return currentSize < spaceToEnd
            ? head + currentSize
            : currentSize - spaceToEnd;
    }

    void advanceHead() noexcept {
        const std::size_t next = headIndex() + 1;
        head_ = static_cast<Index>(next == Capacity ? 0 : next);
    }

    T* pointerAt(std::size_t index) noexcept {
        return std::launder(reinterpret_cast<T*>(storage_[index].bytes));
    }

    const T* pointerAt(std::size_t index) const noexcept {
        return std::launder(
            reinterpret_cast<const T*>(storage_[index].bytes));
    }

    // Slot은 raw storage일 뿐이며 emplace가 성공하기 전에는 T object가 아니다.
    Slot storage_[Capacity];
    Index head_;
    Index size_;
};

} // namespace cms
