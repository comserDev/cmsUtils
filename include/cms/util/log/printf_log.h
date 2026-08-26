#pragma once

#include <cstddef>
#include <cstdio>
#include <utility>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/level.h>
#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

// libc printf semantics를 사용하는 optional producer-side helper다.
// MessageBytes에는 terminating NUL이 포함되며 truncation된 message는 enqueue하지 않는다.
template<
    std::size_t MessageBytes,
    std::size_t QueueCapacity,
    class Clock,
    class Sink,
    class Mutex,
    class Formatter,
    class LevelFilter,
    class FullQueuePolicy,
    class... Args>
Status logf(
    AsyncLogger<
        MessageBytes,
        QueueCapacity,
        Clock,
        Sink,
        Mutex,
        Formatter,
        LevelFilter,
        FullQueuePolicy>& logger,
    Level level,
    const char* format,
    Args&&... args) {
    // Filtered log는 libc formatting과 stack scratch 초기화를 모두 건너뛴다.
    if (!logger.wouldLog(level)) {
        return Status::ok;
    }
    if (format == nullptr) {
        return Status::invalid_argument;
    }

    char formatted[MessageBytes] = {};
    const int produced = std::snprintf(
        formatted,
        sizeof(formatted),
        format,
        std::forward<Args>(args)...);
    if (produced < 0) {
        return Status::invalid_argument;
    }

    const std::size_t payloadSize = static_cast<std::size_t>(produced);
    if (payloadSize >= MessageBytes) {
        return Status::no_space;
    }

    // AsyncLogger가 payload를 StaticRecord로 복사하므로 local buffer는 남지 않는다.
    return logger.log(level, StringView(formatted, payloadSize));
}

} // namespace log
} // namespace util
} // namespace cms
