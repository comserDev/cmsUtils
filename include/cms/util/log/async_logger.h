#pragma once

#include <cstddef>

#include <cms/util/log/detail/async_logger_core.h>
#include <cms/util/log/detail/static_queue_backend.h>
#include <cms/util/log/formatter.h>
#include <cms/util/log/full_queue_policy.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/record.h>

namespace cms {
namespace util {
namespace log {

// Fixed-capacity queue를 직접 소유하는 deterministic AsyncLogger다.
// QueueCapacity와 full 정책을 포함한 기존 public API와 object layout을 유지한다.
template<
    std::size_t MessageBytes,
    std::size_t QueueCapacity,
    class Clock,
    class Sink,
    class Mutex,
    class Formatter = PlainFormatter,
    class LevelFilter = NoLevelFilter,
    class FullQueuePolicy = RejectOnFull>
using AsyncLogger = detail::BasicAsyncLogger<
    MessageBytes,
    Clock,
    Sink,
    Mutex,
    Formatter,
    LevelFilter,
    detail::StaticQueueBackend<
        StaticRecord<MessageBytes>,
        QueueCapacity,
        Mutex,
        FullQueuePolicy>>;

} // namespace log
} // namespace util
} // namespace cms
