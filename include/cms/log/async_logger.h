#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <cms/log/clock.h>
#include <cms/log/formatter.h>
#include <cms/log/level.h>
#include <cms/log/record.h>
#include <cms/log/sink.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/status.h>
#include <cms/string_view.h>
#include <cms/synchronized_queue.h>

namespace cms {
namespace log {

// Queue access는 Mutex가 보호하고 Clock의 producer 동시 호출 안전성은 backend가
// 보장한다. drainOne은 single consumer를 전제로 하며 여러 consumer가 호출하면
// queue pop은 보호되지만 Sink의 동시 write와 최종 출력 순서는 보장하지 않는다.
template<
    std::size_t MessageBytes,
    std::size_t QueueCapacity,
    class Clock,
    class Sink,
    class Mutex>
class AsyncLogger {
    static_assert(
        MessageBytes
            <= (std::numeric_limits<std::size_t>::max)()
                - maxFormattedRecordOverhead,
        "AsyncLogger formatted line storage size overflows size_t");

    using OwnedRecord = StaticRecord<MessageBytes>;
    using RecordQueue = StaticQueue<OwnedRecord, QueueCapacity>;
    using Queue = SynchronizedQueue<RecordQueue, Mutex>;

    static constexpr std::size_t formattedLineStorageBytes =
        MessageBytes + maxFormattedRecordOverhead;

public:
    AsyncLogger() = default;

    template<
        class ClockType = Clock,
        class SinkType = Sink,
        typename std::enable_if<
            std::is_move_constructible<ClockType>::value
                && std::is_move_constructible<SinkType>::value
                && std::is_default_constructible<Queue>::value,
            int>::type = 0>
    AsyncLogger(Clock clock, Sink sink)
        noexcept(
            std::is_nothrow_move_constructible<ClockType>::value
            && std::is_nothrow_move_constructible<SinkType>::value
            && std::is_nothrow_default_constructible<Queue>::value)
        : clock_(std::move(clock)), sink_(std::move(sink)), queue_() {}

    template<
        class ClockType = Clock,
        class SinkType = Sink,
        class MutexType = Mutex,
        typename std::enable_if<
            std::is_move_constructible<ClockType>::value
                && std::is_move_constructible<SinkType>::value
                && std::is_move_constructible<MutexType>::value
                && std::is_constructible<Queue, Mutex>::value,
            int>::type = 0>
    AsyncLogger(Clock clock, Sink sink, Mutex mutex)
        noexcept(
            std::is_nothrow_move_constructible<ClockType>::value
            && std::is_nothrow_move_constructible<SinkType>::value
            && std::is_nothrow_constructible<Queue, Mutex>::value)
        : clock_(std::move(clock)),
          sink_(std::move(sink)),
          queue_(std::move(mutex)) {}

    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    AsyncLogger(AsyncLogger&&) = delete;
    AsyncLogger& operator=(AsyncLogger&&) = delete;

    Status log(Level level, StringView message) {
        if (message.size() > MessageBytes - 1) {
            return Status::no_space;
        }

        const Timestamp timestamp = clock_.nowMilliseconds();
        OwnedRecord record;
        const WriteResult assigned = record.assign(level, timestamp, message);
        if (assigned.status != Status::ok) {
            return assigned.status;
        }

        return queue_.push(std::move(record));
    }

    Status drainOne() {
        OwnedRecord record;
        const Status consumed = queue_.consumeFront(
            [&record](const OwnedRecord& queued) {
                record = queued;
            });
        if (consumed != Status::ok) {
            return consumed;
        }

        StaticString<formattedLineStorageBytes> line;
        const WriteResult formatted = format(record.view(), line.buffer());
        // dequeue가 끝난 뒤 format하므로 실패해도 record는 이미 제거된 상태다.
        if (formatted.status != Status::ok) {
            return formatted.status;
        }

        // queue lock을 풀고 나서 sink I/O를 수행한다.
        sink_.write(line.view());
        return Status::ok;
    }

    std::size_t pending() const
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
    Clock clock_;
    Sink sink_;
    Queue queue_;
};

} // namespace log
} // namespace cms
