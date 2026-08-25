#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/clock.h>
#include <cms/log/formatter.h>
#include <cms/log/level.h>
#include <cms/log/record.h>
#include <cms/static_string.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>

namespace {

struct TestClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct TestSink {
    void write(cms::StringView) noexcept {}
};

struct ExternalMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

struct NonMovableMutex {
    NonMovableMutex() noexcept = default;
    NonMovableMutex(const NonMovableMutex&) = delete;
    NonMovableMutex& operator=(const NonMovableMutex&) = delete;
    NonMovableMutex(NonMovableMutex&&) = delete;
    NonMovableMutex& operator=(NonMovableMutex&&) = delete;

    void lock() noexcept {}
    void unlock() noexcept {}
};

template<class Type, class = void>
struct HasQueueAccessor : std::false_type {};

template<class Type>
struct HasQueueAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().queue())>> : std::true_type {};

template<class Type, class = void>
struct HasSinkAccessor : std::false_type {};

template<class Type>
struct HasSinkAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().sink())>> : std::true_type {};

template<class Type, class = void>
struct HasClockAccessor : std::false_type {};

template<class Type>
struct HasClockAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().clock())>> : std::true_type {};

template<class Type, class = void>
struct HasMutexAccessor : std::false_type {};

template<class Type>
struct HasMutexAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().mutex())>> : std::true_type {};

template<class Type, class = void>
struct HasFrontAccessor : std::false_type {};

template<class Type>
struct HasFrontAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().front())>> : std::true_type {};

using StaticRecord = cms::log::StaticRecord<16>;
using Logger = cms::log::AsyncLogger<
    16, 4, TestClock, TestSink, cms::sync::NullMutex>;
using ExplicitPlainLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::PlainFormatter>;
using AnsiLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::AnsiFormatter>;
using NonMovableLogger = cms::log::AsyncLogger<
    16, 4, TestClock, TestSink, NonMovableMutex>;
using MutexRef = cms::sync::MutexRef<ExternalMutex>;
using ReferencedLogger = cms::log::AsyncLogger<
    16, 4, TestClock, TestSink, MutexRef>;

constexpr StaticRecord recordContract;

} // namespace

static_assert(
    std::is_same<
        std::underlying_type<cms::log::Level>::type,
        std::uint8_t>::value,
    "Level must use uint8_t");
static_assert(std::is_same<cms::log::Timestamp, std::uint64_t>::value,
    "Timestamp must use uint64_t");
static_assert(cms::log::maxFormattedRecordOverhead == 35,
    "formatted overhead contract changed");
static_assert(cms::log::PlainFormatter::maxOverhead == 35,
    "plain formatter overhead contract changed");
static_assert(cms::log::AnsiFormatter::maxOverhead == 43,
    "ANSI formatter overhead contract changed");
static_assert(std::is_same<Logger, ExplicitPlainLogger>::value,
    "five-parameter logger must keep the plain formatter default");
static_assert(!std::is_same<Logger, AnsiLogger>::value,
    "ANSI formatter must require explicit selection");
static_assert(std::is_same<
    decltype(cms::log::PlainFormatter::format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    cms::WriteResult>::value,
    "PlainFormatter must return WriteResult");
static_assert(std::is_same<
    decltype(cms::log::AnsiFormatter::format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    cms::WriteResult>::value,
    "AnsiFormatter must return WriteResult");
static_assert(noexcept(cms::log::PlainFormatter::format(
    std::declval<const cms::log::Record&>(),
    cms::StringBuffer())),
    "PlainFormatter must preserve noexcept");
static_assert(noexcept(cms::log::AnsiFormatter::format(
    std::declval<const cms::log::Record&>(),
    cms::StringBuffer())),
    "AnsiFormatter must preserve noexcept");

static_assert(recordContract.messageCapacity() == 16,
    "message capacity includes the terminating NUL");
static_assert(recordContract.maxMessageSize() == 15,
    "maximum message payload is capacity minus one");
static_assert(std::is_same<
    decltype(std::declval<const StaticRecord&>().level()),
    cms::log::Level>::value, "level has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const StaticRecord&>().timestampMilliseconds()),
    cms::log::Timestamp>::value, "timestamp has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const StaticRecord&>().message()),
    cms::StringView>::value, "message has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const StaticRecord&>().view()),
    cms::log::Record>::value, "view has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<StaticRecord&>().assign(
        cms::log::Level::info,
        cms::log::Timestamp{0},
        cms::StringView())),
    cms::WriteResult>::value, "assign has the wrong return type");

static_assert(std::is_nothrow_default_constructible<Logger>::value,
    "default logger must preserve noexcept construction");
static_assert(std::is_default_constructible<NonMovableLogger>::value,
    "logger must own a non-movable default mutex");
static_assert(std::is_constructible<
    NonMovableLogger, TestClock, TestSink>::value,
    "Clock and Sink constructor must support a non-movable mutex");
static_assert(!std::is_default_constructible<ReferencedLogger>::value,
    "MutexRef logger requires an external mutex");
static_assert(std::is_constructible<
    ReferencedLogger, TestClock, TestSink, MutexRef>::value,
    "MutexRef logger must accept its backend");
static_assert(!std::is_copy_constructible<Logger>::value,
    "logger copy must be deleted");
static_assert(!std::is_copy_assignable<Logger>::value,
    "logger copy assignment must be deleted");
static_assert(!std::is_move_constructible<Logger>::value,
    "logger move must be deleted");
static_assert(!std::is_move_assignable<Logger>::value,
    "logger move assignment must be deleted");

static_assert(std::is_same<
    decltype(std::declval<Logger&>().log(
        cms::log::Level::info, cms::StringView())),
    cms::Status>::value, "log has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<Logger&>().drainOne()),
    cms::Status>::value, "drainOne has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().pending()),
    std::size_t>::value, "pending has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().capacity()),
    std::size_t>::value, "capacity has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().empty()),
    bool>::value, "empty has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().full()),
    bool>::value, "full has the wrong return type");

static_assert(!HasQueueAccessor<Logger>::value,
    "raw Queue access must not escape the logger");
static_assert(!HasSinkAccessor<Logger>::value,
    "raw Sink access must not escape the logger");
static_assert(!HasClockAccessor<Logger>::value,
    "raw Clock access must not escape the logger");
static_assert(!HasMutexAccessor<Logger>::value,
    "raw Mutex access must not escape the logger");
static_assert(!HasFrontAccessor<Logger>::value,
    "raw front access must not escape the logger");
