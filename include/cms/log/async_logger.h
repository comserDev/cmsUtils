#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <cms/log/clock.h>
#include <cms/log/formatter.h>
#include <cms/log/level.h>
#include <cms/log/level_filter.h>
#include <cms/log/record.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/sink.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/static_queue.h>
#include <cms/static_string.h>
#include <cms/status.h>
#include <cms/string_view.h>
#include <cms/synchronized_queue.h>

namespace cms {
namespace log {

namespace detail {

template<class Formatter>
struct IsRuntimeColorFormatter : std::false_type {};

template<>
struct IsRuntimeColorFormatter<RuntimeAnsiFormatter> : std::true_type {};

template<>
struct IsRuntimeColorFormatter<RuntimeStyledAnsiFormatter>
    : std::true_type {};

template<class Formatter>
struct IsStyledFormatter : std::false_type {};

template<>
struct IsStyledFormatter<StyledAnsiFormatter> : std::true_type {};

template<>
struct IsStyledFormatter<RuntimeStyledAnsiFormatter> : std::true_type {};

template<
    std::size_t MessageBytes,
    class Formatter,
    bool Styled = IsStyledFormatter<Formatter>::value>
struct AsyncLoggerLineStorage {
    static_assert(
        MessageBytes
            <= (std::numeric_limits<std::size_t>::max)()
                - Formatter::maxOverhead,
        "AsyncLogger formatted line storage size overflows size_t");

    static constexpr std::size_t value =
        MessageBytes + Formatter::maxOverhead;
};

template<std::size_t MessageBytes, class Formatter>
struct AsyncLoggerLineStorage<MessageBytes, Formatter, true> {
    static_assert(
        styledFormattedStorageAdjustment
            <= (std::numeric_limits<std::size_t>::max)(),
        "AsyncLogger styled line adjustment overflows size_t");
    static_assert(
        MessageBytes
            <= ((std::numeric_limits<std::size_t>::max)()
                - styledFormattedStorageAdjustment)
                / maxStyledMessageExpansionFactor,
        "AsyncLogger styled line storage size overflows size_t");

    // max message는 MessageBytes - 1이고 line의 NUL까지 포함하면 4N + 40이다.
    static constexpr std::size_t value =
        MessageBytes * maxStyledMessageExpansionFactor
        + styledFormattedStorageAdjustment;
};

// 기본 조합은 state를 갖지 않고 두 static policy를 직접 호출한다.
template<
    class Formatter,
    class LevelFilter,
    bool RuntimeColor = IsRuntimeColorFormatter<Formatter>::value,
    bool RuntimeLevel = std::is_same<LevelFilter, RuntimeLevelFilter>::value>
class AsyncLoggerPolicyStorage {
protected:
    WriteResult formatRecord(
        const Record& record,
        StringBuffer output) noexcept(
            noexcept(Formatter::format(record, output))) {
        return Formatter::format(record, output);
    }

    bool allowsLevel(Level level) const noexcept(
        noexcept(LevelFilter::allows(level))) {
        return LevelFilter::allows(level);
    }
};

// Runtime color만 선택한 조합은 formatter state만 소유한다.
template<class Formatter, class LevelFilter>
class AsyncLoggerPolicyStorage<Formatter, LevelFilter, true, false> {
protected:
    WriteResult formatRecord(
        const Record& record,
        StringBuffer output) noexcept {
        return formatter_.format(record, output);
    }

    bool allowsLevel(Level level) const noexcept(
        noexcept(LevelFilter::allows(level))) {
        return LevelFilter::allows(level);
    }

    void setRuntimeUseColor(bool enabled) noexcept {
        formatter_.setUseColor(enabled);
    }

    bool runtimeUseColor() const noexcept {
        return formatter_.useColor();
    }

private:
    Formatter formatter_;
};

// Runtime level만 선택한 조합은 filter state만 소유한다.
template<class Formatter>
class AsyncLoggerPolicyStorage<Formatter, RuntimeLevelFilter, false, true> {
protected:
    WriteResult formatRecord(
        const Record& record,
        StringBuffer output) noexcept(
            noexcept(Formatter::format(record, output))) {
        return Formatter::format(record, output);
    }

    bool allowsLevel(Level level) const noexcept {
        return levelFilter_.allows(level);
    }

    void setRuntimeMinLevel(Level level) noexcept {
        levelFilter_.setMinLevel(level);
    }

    Level runtimeMinLevel() const noexcept {
        return levelFilter_.minLevel();
    }

    void setRuntimeLoggingEnabled(bool enabled) noexcept {
        levelFilter_.setEnabled(enabled);
    }

    bool runtimeLoggingEnabled() const noexcept {
        return levelFilter_.enabled();
    }

private:
    RuntimeLevelFilter levelFilter_;
};

// 두 runtime 기능을 함께 선택한 조합만 두 state를 모두 소유한다.
template<class Formatter>
class AsyncLoggerPolicyStorage<Formatter, RuntimeLevelFilter, true, true> {
protected:
    WriteResult formatRecord(
        const Record& record,
        StringBuffer output) noexcept {
        return formatter_.format(record, output);
    }

    bool allowsLevel(Level level) const noexcept {
        return levelFilter_.allows(level);
    }

    void setRuntimeUseColor(bool enabled) noexcept {
        formatter_.setUseColor(enabled);
    }

    bool runtimeUseColor() const noexcept {
        return formatter_.useColor();
    }

    void setRuntimeMinLevel(Level level) noexcept {
        levelFilter_.setMinLevel(level);
    }

    Level runtimeMinLevel() const noexcept {
        return levelFilter_.minLevel();
    }

    void setRuntimeLoggingEnabled(bool enabled) noexcept {
        levelFilter_.setEnabled(enabled);
    }

    bool runtimeLoggingEnabled() const noexcept {
        return levelFilter_.enabled();
    }

private:
    Formatter formatter_;
    RuntimeLevelFilter levelFilter_;
};

} // namespace detail

// Queue access는 Mutex가 보호하고 Clock의 producer 동시 호출 안전성은 backend가
// 보장한다. drainOne은 single consumer를 전제로 하며 여러 consumer가 호출하면
// queue pop은 보호되지만 Sink의 동시 write와 최종 출력 순서는 보장하지 않는다.
template<
    std::size_t MessageBytes,
    std::size_t QueueCapacity,
    class Clock,
    class Sink,
    class Mutex,
    class Formatter = PlainFormatter,
    class LevelFilter = NoLevelFilter>
class AsyncLogger
    : private detail::AsyncLoggerPolicyStorage<Formatter, LevelFilter> {
    using OwnedRecord = StaticRecord<MessageBytes>;
    using RecordQueue = StaticQueue<OwnedRecord, QueueCapacity>;
    using Queue = SynchronizedQueue<RecordQueue, Mutex>;

    static constexpr std::size_t formattedLineStorageBytes =
        detail::AsyncLoggerLineStorage<MessageBytes, Formatter>::value;

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
        // Level filter는 enqueue 시점에 적용하고 runtime color는 drain 시점에 적용한다.
        if (!this->allowsLevel(level)) {
            return Status::ok;
        }
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
        const WriteResult formatted =
            this->formatRecord(record.view(), line.buffer());
        // dequeue가 끝난 뒤 format하므로 실패해도 record는 이미 제거된 상태다.
        if (formatted.status != Status::ok) {
            return formatted.status;
        }

        // queue lock을 풀고 나서 sink I/O를 수행한다.
        sink_.write(line.view());
        return Status::ok;
    }

    // mode는 drain 시점에 적용한다. drainOne과 동시에 변경하려면 외부 동기화한다.
    template<
        class FormatterType = Formatter,
        typename std::enable_if<
            detail::IsRuntimeColorFormatter<Formatter>::value
                && std::is_same<FormatterType, Formatter>::value,
            int>::type = 0>
    void setUseColor(bool enabled) noexcept {
        this->setRuntimeUseColor(enabled);
    }

    template<
        class FormatterType = Formatter,
        typename std::enable_if<
            detail::IsRuntimeColorFormatter<Formatter>::value
                && std::is_same<FormatterType, Formatter>::value,
            int>::type = 0>
    bool useColor() const noexcept {
        return this->runtimeUseColor();
    }

    // Runtime filter 설정과 log를 동시에 호출하려면 caller가 외부에서 동기화한다.
    template<
        class LevelFilterType = LevelFilter,
        typename std::enable_if<
            std::is_same<LevelFilter, RuntimeLevelFilter>::value
                && std::is_same<LevelFilterType, LevelFilter>::value,
            int>::type = 0>
    void setMinLevel(Level level) noexcept {
        this->setRuntimeMinLevel(level);
    }

    template<
        class LevelFilterType = LevelFilter,
        typename std::enable_if<
            std::is_same<LevelFilter, RuntimeLevelFilter>::value
                && std::is_same<LevelFilterType, LevelFilter>::value,
            int>::type = 0>
    Level minLevel() const noexcept {
        return this->runtimeMinLevel();
    }

    template<
        class LevelFilterType = LevelFilter,
        typename std::enable_if<
            std::is_same<LevelFilter, RuntimeLevelFilter>::value
                && std::is_same<LevelFilterType, LevelFilter>::value,
            int>::type = 0>
    void setLoggingEnabled(bool enabled) noexcept {
        this->setRuntimeLoggingEnabled(enabled);
    }

    template<
        class LevelFilterType = LevelFilter,
        typename std::enable_if<
            std::is_same<LevelFilter, RuntimeLevelFilter>::value
                && std::is_same<LevelFilterType, LevelFilter>::value,
            int>::type = 0>
    bool loggingEnabled() const noexcept {
        return this->runtimeLoggingEnabled();
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
