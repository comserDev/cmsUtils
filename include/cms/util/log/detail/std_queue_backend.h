#pragma once

#include <cstddef>
#include <memory>
#include <queue>
#include <type_traits>
#include <utility>

#include <cms/util/status.h>
#include <cms/util/sync/synchronized_queue.h>

namespace cms {
namespace util {
namespace log {
namespace detail {

// std::queue를 SynchronizedQueue가 요구하는 작은 Status 기반 API로 맞춘다.
// 할당 실패를 포함한 std::queue exception은 그대로 caller에게 전파한다.
template<class Record>
class StdQueueAdapter {
public:
    std::size_t size() const noexcept(noexcept(queue_.size())) {
        return queue_.size();
    }

    bool empty() const noexcept(noexcept(queue_.empty())) {
        return queue_.empty();
    }

    Record* front()
        noexcept(noexcept(queue_.empty()) && noexcept(queue_.front())) {
        return queue_.empty() ? nullptr : std::addressof(queue_.front());
    }

    const Record* front() const
        noexcept(noexcept(queue_.empty()) && noexcept(queue_.front())) {
        return queue_.empty() ? nullptr : std::addressof(queue_.front());
    }

    Status push(const Record& record) {
        queue_.push(record);
        return Status::ok;
    }

    Status push(Record&& record) {
        queue_.push(std::move(record));
        return Status::ok;
    }

    template<class... Args>
    Status emplace(Args&&... args) {
        queue_.emplace(std::forward<Args>(args)...);
        return Status::ok;
    }

    Status pop()
        noexcept(noexcept(queue_.empty()) && noexcept(queue_.pop())) {
        if (queue_.empty()) {
            return Status::out_of_range;
        }

        queue_.pop();
        return Status::ok;
    }

private:
    std::queue<Record> queue_;
};

// Host용 dynamic queue의 ownership과 synchronization을 한 lifetime으로 묶는다.
template<class Record, class Mutex>
class StdQueueBackend {
    using RecordQueue = StdQueueAdapter<Record>;
    using Queue = sync::SynchronizedQueue<RecordQueue, Mutex>;

public:
    static constexpr bool isBounded = false;

    StdQueueBackend() = default;

    template<
        class MutexType = Mutex,
        typename std::enable_if<
            std::is_move_constructible<MutexType>::value
                && std::is_constructible<Queue, Mutex>::value,
            int>::type = 0>
    explicit StdQueueBackend(Mutex mutex)
        noexcept(std::is_nothrow_constructible<Queue, Mutex>::value)
        : queue_(std::move(mutex)) {}

    Status enqueue(Record&& record) {
        return queue_.push(std::move(record));
    }

    template<class Consumer>
    Status consumeFront(Consumer&& consumer) {
        return queue_.consumeFront(std::forward<Consumer>(consumer));
    }

    std::size_t size() const
        noexcept(noexcept(std::declval<const Queue&>().size())) {
        return queue_.size();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const Queue&>().empty())) {
        return queue_.empty();
    }

private:
    Queue queue_;
};

} // namespace detail
} // namespace log
} // namespace util
} // namespace cms
