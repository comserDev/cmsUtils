#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/util/log/full_queue_policy.h>
#include <cms/util/static_queue.h>
#include <cms/util/status.h>
#include <cms/util/sync/synchronized_queue.h>

namespace cms {
namespace util {
namespace log {
namespace detail {

// 고정 용량 queue의 저장소와 full 정책을 logging core 밖에서 처리한다.
template<class Record, std::size_t Capacity, class Mutex, class FullQueuePolicy>
class StaticQueueBackend {
    using RecordQueue = StaticQueue<Record, Capacity>;
    using Queue = sync::SynchronizedQueue<RecordQueue, Mutex>;

    static_assert(
        std::is_same<FullQueuePolicy, RejectOnFull>::value
            || std::is_same<FullQueuePolicy, OverwriteOldestOnFull>::value,
        "AsyncLogger supports only built-in full queue policies");

public:
    static constexpr bool isBounded = true;

    StaticQueueBackend() = default;

    template<
        class MutexType = Mutex,
        typename std::enable_if<
            std::is_move_constructible<MutexType>::value
                && std::is_constructible<Queue, Mutex>::value,
            int>::type = 0>
    explicit StaticQueueBackend(Mutex mutex)
        noexcept(std::is_nothrow_constructible<Queue, Mutex>::value)
        : queue_(std::move(mutex)) {}

    Status enqueue(Record&& record)
        noexcept(
            std::is_same<FullQueuePolicy, OverwriteOldestOnFull>::value
                ? noexcept(std::declval<Queue&>().pushOverwrite(
                    std::declval<Record&&>()))
                : noexcept(std::declval<Queue&>().push(
                    std::declval<Record&&>()))) {
        if constexpr (
            std::is_same<FullQueuePolicy, OverwriteOldestOnFull>::value) {
            return queue_.pushOverwrite(std::move(record));
        } else {
            return queue_.push(std::move(record));
        }
    }

    template<class Consumer>
    Status consumeFront(Consumer&& consumer) {
        return queue_.consumeFront(std::forward<Consumer>(consumer));
    }

    std::size_t size() const
        noexcept(noexcept(std::declval<const Queue&>().size())) {
        return queue_.size();
    }

    std::size_t capacity() const
        noexcept(noexcept(std::declval<const Queue&>().capacity())) {
        return queue_.capacity();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const Queue&>().empty())) {
        return queue_.empty();
    }

    bool full() const
        noexcept(noexcept(std::declval<const Queue&>().full())) {
        return queue_.full();
    }

private:
    Queue queue_;
};

} // namespace detail
} // namespace log
} // namespace util
} // namespace cms
