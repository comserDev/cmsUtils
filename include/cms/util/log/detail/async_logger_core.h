#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <cms/util/log/clock.h>
#include <cms/util/log/formatter.h>
#include <cms/util/log/level.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/record.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/sink.h>
#include <cms/util/log/styled_ansi_formatter.h>
#include <cms/util/log/utc_offset_formatter.h>
#include <cms/util/static_string.h>
#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
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
struct IsRuntimeColorFormatter<UtcOffsetFormatter<Formatter>>
    : IsRuntimeColorFormatter<Formatter> {};

template<class Formatter>
struct IsStatefulFormatter : std::false_type {};

template<class Formatter>
struct IsStatefulFormatter<UtcOffsetFormatter<Formatter>>
    : std::true_type {};

template<class Formatter>
struct IsRuntimeUtcOffsetFormatter : std::false_type {};

template<class Formatter>
struct IsRuntimeUtcOffsetFormatter<UtcOffsetFormatter<Formatter>>
    : std::true_type {};

template<class Formatter>
struct IsStyledFormatter : std::false_type {};

template<>
struct IsStyledFormatter<StyledAnsiFormatter> : std::true_type {};

template<>
struct IsStyledFormatter<RuntimeStyledAnsiFormatter> : std::true_type {};

template<class Formatter>
struct IsStyledFormatter<UtcOffsetFormatter<Formatter>>
    : IsStyledFormatter<Formatter> {};

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
    bool RuntimeLevel = std::is_same<LevelFilter, RuntimeLevelFilter>::value,
    bool Stateful = IsStatefulFormatter<Formatter>::value>
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
template<class Formatter, class LevelFilter, bool Stateful>
class AsyncLoggerPolicyStorage<
    Formatter,
    LevelFilter,
    true,
    false,
    Stateful> {
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

    Status setRuntimeUtcOffsetMinutes(int minutes) noexcept {
        return formatter_.setUtcOffsetMinutes(minutes);
    }

    int runtimeUtcOffsetMinutes() const noexcept {
        return formatter_.utcOffsetMinutes();
    }

private:
    Formatter formatter_;
};

// Runtime level만 선택한 조합은 filter state만 소유한다.
template<class Formatter>
class AsyncLoggerPolicyStorage<
    Formatter,
    RuntimeLevelFilter,
    false,
    true,
    false> {
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

// Runtime color/level 없이 stateful formatter만 선택한 조합은 formatter state만 소유한다.
template<class Formatter, class LevelFilter>
class AsyncLoggerPolicyStorage<
    Formatter,
    LevelFilter,
    false,
    false,
    true> {
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

    Status setRuntimeUtcOffsetMinutes(int minutes) noexcept {
        return formatter_.setUtcOffsetMinutes(minutes);
    }

    int runtimeUtcOffsetMinutes() const noexcept {
        return formatter_.utcOffsetMinutes();
    }

private:
    Formatter formatter_;
};

// Runtime level과 stateful formatter 조합은 formatter와 filter state를 모두 소유한다.
template<class Formatter>
class AsyncLoggerPolicyStorage<
    Formatter,
    RuntimeLevelFilter,
    false,
    true,
    true> {
protected:
    WriteResult formatRecord(
        const Record& record,
        StringBuffer output) noexcept {
        return formatter_.format(record, output);
    }

    bool allowsLevel(Level level) const noexcept {
        return levelFilter_.allows(level);
    }

    Status setRuntimeUtcOffsetMinutes(int minutes) noexcept {
        return formatter_.setUtcOffsetMinutes(minutes);
    }

    int runtimeUtcOffsetMinutes() const noexcept {
        return formatter_.utcOffsetMinutes();
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

// 두 runtime 기능을 함께 선택한 조합만 formatter와 filter state를 모두 소유한다.
template<class Formatter, bool Stateful>
class AsyncLoggerPolicyStorage<
    Formatter,
    RuntimeLevelFilter,
    true,
    true,
    Stateful> {
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

    Status setRuntimeUtcOffsetMinutes(int minutes) noexcept {
        return formatter_.setUtcOffsetMinutes(minutes);
    }

    int runtimeUtcOffsetMinutes() const noexcept {
        return formatter_.utcOffsetMinutes();
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

// Queue backend는 storage와 synchronization만 담당하며 logging algorithm은
// fixed/dynamic storage 차이를 알지 않는다.
template<
    std::size_t MessageBytes,
    class Clock,
    class Sink,
    class Mutex,
    class Formatter,
    class LevelFilter,
    class QueueBackend>
class BasicAsyncLogger
    : private detail::AsyncLoggerPolicyStorage<Formatter, LevelFilter> {
    using OwnedRecord = StaticRecord<MessageBytes>;
    using Queue = QueueBackend;

    static constexpr std::size_t formattedLineStorageBytes =
        detail::AsyncLoggerLineStorage<MessageBytes, Formatter>::value;

public:
    static constexpr std::size_t messageCapacity() noexcept {
        return MessageBytes;
    }

    BasicAsyncLogger() = default;

    template<
        class ClockType = Clock,
        class SinkType = Sink,
        typename std::enable_if<
            std::is_move_constructible<ClockType>::value
                && std::is_move_constructible<SinkType>::value
                && std::is_default_constructible<Queue>::value,
            int>::type = 0>
    BasicAsyncLogger(Clock clock, Sink sink)
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
    BasicAsyncLogger(Clock clock, Sink sink, Mutex mutex)
        noexcept(
            std::is_nothrow_move_constructible<ClockType>::value
            && std::is_nothrow_move_constructible<SinkType>::value
            && std::is_nothrow_constructible<Queue, Mutex>::value)
        : clock_(std::move(clock)),
          sink_(std::move(sink)),
          queue_(std::move(mutex)) {}

    BasicAsyncLogger(const BasicAsyncLogger&) = delete;
    BasicAsyncLogger& operator=(const BasicAsyncLogger&) = delete;
    BasicAsyncLogger(BasicAsyncLogger&&) = delete;
    BasicAsyncLogger& operator=(BasicAsyncLogger&&) = delete;

    // Queue나 Clock을 건드리지 않고 현재 LevelFilter policy만 조회한다.
    // Runtime 설정과 동시에 호출하려면 기존 filter contract대로 외부 동기화한다.
    bool wouldLog(Level level) const
        noexcept(noexcept(this->allowsLevel(level))) {
        return this->allowsLevel(level);
    }

    Status log(Level level, StringView message) {
        // Level filter는 enqueue 시점에 적용하고 runtime color는 drain 시점에 적용한다.
        if (!wouldLog(level)) {
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

        return queue_.enqueue(std::move(record));
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

        // queue lock을 풀고 나서 sink I/O를 수행한다. 실패해도 record는 requeue하지 않는다.
        return sink_.write(line.view());
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

    // offset은 formatter state이며 이미 enqueue된 record에도 drain 시점 값을 적용한다.
    template<
        class FormatterType = Formatter,
        typename std::enable_if<
            detail::IsRuntimeUtcOffsetFormatter<Formatter>::value
                && std::is_same<FormatterType, Formatter>::value,
            int>::type = 0>
    Status setUtcOffsetMinutes(int minutes) noexcept {
        return this->setRuntimeUtcOffsetMinutes(minutes);
    }

    template<
        class FormatterType = Formatter,
        typename std::enable_if<
            detail::IsRuntimeUtcOffsetFormatter<Formatter>::value
                && std::is_same<FormatterType, Formatter>::value,
            int>::type = 0>
    int utcOffsetMinutes() const noexcept {
        return this->runtimeUtcOffsetMinutes();
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

    template<
        class QueueType = Queue,
        typename std::enable_if<QueueType::isBounded, int>::type = 0>
    std::size_t capacity() const
        noexcept(noexcept(std::declval<const QueueType&>().capacity())) {
        return queue_.capacity();
    }

    bool empty() const
        noexcept(noexcept(std::declval<const Queue&>().empty())) {
        return queue_.empty();
    }

    template<
        class QueueType = Queue,
        typename std::enable_if<QueueType::isBounded, int>::type = 0>
    bool full() const
        noexcept(noexcept(std::declval<const QueueType&>().full())) {
        return queue_.full();
    }

private:
    Clock clock_;
    Sink sink_;
    Queue queue_;
};

} // namespace detail
} // namespace log
} // namespace util
} // namespace cms
