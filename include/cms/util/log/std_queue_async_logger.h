#pragma once

#include <cstddef>

#include <cms/util/log/detail/async_logger_core.h>
#include <cms/util/log/detail/std_queue_backend.h>
#include <cms/util/log/formatter.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/record.h>

namespace cms {
namespace util {
namespace log {

// std::queue의 dynamic storage를 명시적으로 선택하는 host용 AsyncLogger다.
// capacity/full/overwrite 정책은 제공하지 않으며 allocation exception은 전파한다.
template<
    std::size_t MessageBytes,
    class Clock,
    class Sink,
    class Mutex,
    class Formatter = PlainFormatter,
    class LevelFilter = NoLevelFilter>
using StdQueueAsyncLogger = detail::BasicAsyncLogger<
    MessageBytes,
    Clock,
    Sink,
    Mutex,
    Formatter,
    LevelFilter,
    detail::StdQueueBackend<StaticRecord<MessageBytes>, Mutex>>;

} // namespace log
} // namespace util
} // namespace cms
