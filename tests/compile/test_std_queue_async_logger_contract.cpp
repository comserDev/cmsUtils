#include <cstddef>
#include <queue>
#include <type_traits>
#include <utility>

#include <cms/util/log/async_logger.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/std_queue_async_logger.h>
#include <cms/util/sync/null_mutex.h>

namespace {

struct ContractClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct ContractSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
};

template<class T, class = void>
struct HasCapacity : std::false_type {};

template<class T>
struct HasCapacity<
    T,
    std::void_t<decltype(std::declval<const T&>().capacity())>>
    : std::true_type {};

template<class T, class = void>
struct HasFull : std::false_type {};

template<class T>
struct HasFull<T, std::void_t<decltype(std::declval<const T&>().full())>>
    : std::true_type {};

using StaticLogger = cms::util::log::AsyncLogger<
    16,
    4,
    ContractClock,
    ContractSink,
    cms::util::sync::NullMutex>;
using OverwriteLogger = cms::util::log::AsyncLogger<
    16,
    4,
    ContractClock,
    ContractSink,
    cms::util::sync::NullMutex,
    cms::util::log::PlainFormatter,
    cms::util::log::NoLevelFilter,
    cms::util::log::OverwriteOldestOnFull>;
using StdLogger = cms::util::log::StdQueueAsyncLogger<
    16,
    ContractClock,
    ContractSink,
    cms::util::sync::NullMutex>;
using RuntimeStdLogger = cms::util::log::StdQueueAsyncLogger<
    16,
    ContractClock,
    ContractSink,
    cms::util::sync::NullMutex,
    cms::util::log::RuntimeAnsiFormatter,
    cms::util::log::RuntimeLevelFilter>;
using StdRecord = cms::util::log::StaticRecord<16>;
using StdAdapter = cms::util::log::detail::StdQueueAdapter<StdRecord>;
using StandardQueue = std::queue<StdRecord>;

static_assert(
    noexcept(std::declval<const StdAdapter&>().size())
        == noexcept(std::declval<const StandardQueue&>().size()),
    "adapter size must preserve the underlying noexcept contract");
static_assert(
    noexcept(std::declval<const StdAdapter&>().empty())
        == noexcept(std::declval<const StandardQueue&>().empty()),
    "adapter empty must preserve the underlying noexcept contract");
static_assert(
    noexcept(std::declval<StdAdapter&>().front())
        == (noexcept(std::declval<const StandardQueue&>().empty())
            && noexcept(std::declval<StandardQueue&>().front())),
    "adapter front must include every underlying operation");
static_assert(
    noexcept(std::declval<StdAdapter&>().pop())
        == (noexcept(std::declval<const StandardQueue&>().empty())
            && noexcept(std::declval<StandardQueue&>().pop())),
    "adapter pop must include every underlying operation");

static_assert(HasCapacity<StaticLogger>::value, "static logger has capacity");
static_assert(HasFull<StaticLogger>::value, "static logger has full state");
static_assert(!HasCapacity<StdLogger>::value, "std logger has no capacity");
static_assert(!HasFull<StdLogger>::value, "std logger has no full state");
static_assert(
    std::is_default_constructible<StaticLogger>::value,
    "existing static logger remains default constructible");
static_assert(
    std::is_default_constructible<OverwriteLogger>::value,
    "overwrite policy remains available on static logger");
static_assert(
    std::is_default_constructible<StdLogger>::value,
    "std logger is default constructible with default components");
static_assert(
    StdLogger::messageCapacity() == 16,
    "std logger exposes compile-time message capacity");
static_assert(
    std::is_same<
        decltype(std::declval<StdLogger&>().log(
            cms::util::log::Level::info,
            cms::util::StringView())),
        cms::util::Status>::value,
    "std logger log returns Status");
static_assert(
    std::is_same<
        decltype(std::declval<StdLogger&>().drainOne()),
        cms::util::Status>::value,
    "std logger drainOne returns Status");
static_assert(
    std::is_same<
        decltype(std::declval<const StdLogger&>().pending()),
        std::size_t>::value,
    "std logger pending returns size_t");
static_assert(
    std::is_same<
        decltype(std::declval<RuntimeStdLogger&>().setUseColor(true)),
        void>::value,
    "runtime formatter API remains available");
static_assert(
    std::is_same<
        decltype(std::declval<RuntimeStdLogger&>().setMinLevel(
            cms::util::log::Level::warning)),
        void>::value,
    "runtime level API remains available");

} // namespace
