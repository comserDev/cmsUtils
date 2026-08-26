#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/util/status.h>
#include <cms/util/sync/lock_guard.h>

namespace cms {
namespace util {
namespace sync {

// Queue와 Mutex를 값으로 소유하며 모든 mutable queue operation을 lock으로 감싼다.
template<class Queue, class Mutex>
class SynchronizedQueue {
public:
    SynchronizedQueue() = default;

    template<
        class MutexType = Mutex,
        typename std::enable_if<
            std::is_default_constructible<Queue>::value
                && std::is_move_constructible<MutexType>::value,
            int>::type = 0>
    explicit SynchronizedQueue(Mutex mutex)
        noexcept(
            std::is_nothrow_default_constructible<Queue>::value
            && std::is_nothrow_move_constructible<MutexType>::value)
        : queue_(), mutex_(std::move(mutex)) {}

    SynchronizedQueue(const SynchronizedQueue&) = delete;
    SynchronizedQueue& operator=(const SynchronizedQueue&) = delete;
    SynchronizedQueue(SynchronizedQueue&&) = delete;
    SynchronizedQueue& operator=(SynchronizedQueue&&) = delete;

    std::size_t size() const
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<const Queue&>().size())
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.size();
    }

    // capacity는 immutable Queue contract에 따라 lock 없이 조회한다.
    // runtime에 capacity가 바뀌는 Queue backend는 지원하지 않는다.
    std::size_t capacity() const
        noexcept(noexcept(std::declval<const Queue&>().capacity())) {
        return queue_.capacity();
    }

    bool empty() const
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<const Queue&>().empty())
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.empty();
    }

    bool full() const
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<const Queue&>().full())
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.full();
    }

    template<class Value>
    Status push(Value&& value)
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<Queue&>().push(
                std::declval<Value&&>()))
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.push(std::forward<Value>(value));
    }

    // full 확인, old element 제거, 새 element 생성까지 하나의 lock으로 보호한다.
    template<class Value>
    Status pushOverwrite(Value&& value)
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<Queue&>().pushOverwrite(
                std::declval<Value&&>()))
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.pushOverwrite(std::forward<Value>(value));
    }

    template<class... Args>
    Status emplace(Args&&... args)
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<Queue&>().emplace(
                std::declval<Args&&>()...))
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.emplace(std::forward<Args>(args)...);
    }

    Status pop()
        noexcept(
            noexcept(std::declval<Mutex&>().lock())
            && noexcept(std::declval<Queue&>().pop())
            && noexcept(std::declval<Mutex&>().unlock())) {
        LockGuard<Mutex> guard(mutex_);
        return queue_.pop();
    }

    // callback을 실행하는 동안에만 front reference와 lock이 유효하다.
    // callback에서 reference를 보관하거나 오래 걸리는 작업과 재귀 lock을 피해야 한다.
    template<class Consumer>
    Status consumeFront(Consumer&& consumer) {
        LockGuard<Mutex> guard(mutex_);
        auto* const element = queue_.front();
        if (element == nullptr) {
            return Status::out_of_range;
        }

        std::forward<Consumer>(consumer)(*element);
        return queue_.pop();
    }

private:
    Queue queue_;
    mutable Mutex mutex_;
};

} // namespace sync
} // namespace util
} // namespace cms
