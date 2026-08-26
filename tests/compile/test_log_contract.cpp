#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/clock.h>
#include <cms/log/formatter.h>
#include <cms/log/level.h>
#include <cms/log/level_filter.h>
#include <cms/log/record.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/platform/std_mutex.h>
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

template<class Type, class = void>
struct HasSetUseColor : std::false_type {};

template<class Type>
struct HasSetUseColor<
    Type,
    std::void_t<decltype(std::declval<Type&>().setUseColor(true))>>
    : std::true_type {};

template<class Type, class = void>
struct HasUseColor : std::false_type {};

template<class Type>
struct HasUseColor<
    Type,
    std::void_t<decltype(std::declval<const Type&>().useColor())>>
    : std::true_type {};

template<class Type, class = void>
struct HasFormatterAccessor : std::false_type {};

template<class Type>
struct HasFormatterAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().formatter())>>
    : std::true_type {};

template<class Type, class = void>
struct HasSetMinLevel : std::false_type {};

template<class Type>
struct HasSetMinLevel<
    Type,
    std::void_t<decltype(std::declval<Type&>().setMinLevel(
        cms::log::Level::warning))>> : std::true_type {};

template<class Type, class = void>
struct HasMinLevel : std::false_type {};

template<class Type>
struct HasMinLevel<
    Type,
    std::void_t<decltype(std::declval<const Type&>().minLevel())>>
    : std::true_type {};

template<class Type, class = void>
struct HasLevelFilterAccessor : std::false_type {};

template<class Type>
struct HasLevelFilterAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().levelFilter())>>
    : std::true_type {};

template<class Type, class = void>
struct HasSetLoggingEnabled : std::false_type {};

template<class Type>
struct HasSetLoggingEnabled<
    Type,
    std::void_t<decltype(std::declval<Type&>().setLoggingEnabled(true))>>
    : std::true_type {};

template<class Type, class = void>
struct HasLoggingEnabled : std::false_type {};

template<class Type>
struct HasLoggingEnabled<
    Type,
    std::void_t<decltype(std::declval<const Type&>().loggingEnabled())>>
    : std::true_type {};

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
using ExplicitPlainNoFilterLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::PlainFormatter,
    cms::log::NoLevelFilter>;
using AnsiLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::AnsiFormatter>;
using ExplicitAnsiNoFilterLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::AnsiFormatter,
    cms::log::NoLevelFilter>;
using RuntimeAnsiLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::RuntimeAnsiFormatter>;
using ExplicitRuntimeAnsiNoFilterLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::RuntimeAnsiFormatter,
    cms::log::NoLevelFilter>;
using StyledLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::StyledAnsiFormatter>;
using RuntimeStyledLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::RuntimeStyledAnsiFormatter>;
using RuntimeLevelPlainLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::PlainFormatter,
    cms::log::RuntimeLevelFilter>;
using RuntimeLevelAnsiLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::AnsiFormatter,
    cms::log::RuntimeLevelFilter>;
using RuntimeLevelRuntimeAnsiLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::RuntimeAnsiFormatter,
    cms::log::RuntimeLevelFilter>;
using RuntimeLevelRuntimeStyledLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    cms::log::RuntimeStyledAnsiFormatter,
    cms::log::RuntimeLevelFilter>;
using StdMutexRuntimeLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::platform::StdMutex,
    cms::log::RuntimeAnsiFormatter>;
using StdMutexRuntimeLevelLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    cms::platform::StdMutex,
    cms::log::PlainFormatter,
    cms::log::RuntimeLevelFilter>;
using NonMovableLogger = cms::log::AsyncLogger<
    16, 4, TestClock, TestSink, NonMovableMutex>;
using NonMovableRuntimeLevelLogger = cms::log::AsyncLogger<
    16,
    4,
    TestClock,
    TestSink,
    NonMovableMutex,
    cms::log::PlainFormatter,
    cms::log::RuntimeLevelFilter>;
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
static_assert(static_cast<std::uint8_t>(cms::log::Level::trace) == 0,
    "trace ordering changed");
static_assert(static_cast<std::uint8_t>(cms::log::Level::debug) == 1,
    "debug ordering changed");
static_assert(static_cast<std::uint8_t>(cms::log::Level::info) == 2,
    "info ordering changed");
static_assert(static_cast<std::uint8_t>(cms::log::Level::warning) == 3,
    "warning ordering changed");
static_assert(static_cast<std::uint8_t>(cms::log::Level::error) == 4,
    "error ordering changed");
static_assert(static_cast<std::uint8_t>(cms::log::Level::critical) == 5,
    "critical ordering changed");
static_assert(std::is_same<cms::log::Timestamp, std::uint64_t>::value,
    "Timestamp must use uint64_t");
static_assert(cms::log::maxFormattedRecordOverhead == 35,
    "formatted overhead contract changed");
static_assert(cms::log::PlainFormatter::maxOverhead == 35,
    "plain formatter overhead contract changed");
static_assert(cms::log::AnsiFormatter::maxOverhead == 43,
    "ANSI formatter overhead contract changed");
static_assert(cms::log::RuntimeAnsiFormatter::maxOverhead == 43,
    "runtime ANSI formatter overhead contract changed");
static_assert(cms::log::maxStyledMessageExpansionFactor == 4,
    "styled message expansion contract changed");
static_assert(cms::log::styledFormattedStorageAdjustment == 40,
    "styled line storage adjustment changed");
static_assert(std::is_same<Logger, ExplicitPlainLogger>::value,
    "five-parameter logger must keep the plain formatter default");
static_assert(std::is_same<Logger, ExplicitPlainNoFilterLogger>::value,
    "five-parameter logger must keep the no-filter default");
static_assert(!std::is_same<Logger, AnsiLogger>::value,
    "ANSI formatter must require explicit selection");
static_assert(std::is_same<AnsiLogger, ExplicitAnsiNoFilterLogger>::value,
    "six-parameter ANSI logger must keep the no-filter default");
static_assert(!std::is_same<Logger, RuntimeAnsiLogger>::value,
    "runtime ANSI formatter must require explicit selection");
static_assert(std::is_same<
    RuntimeAnsiLogger,
    ExplicitRuntimeAnsiNoFilterLogger>::value,
    "six-parameter runtime ANSI logger must keep the no-filter default");
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
static_assert(std::is_nothrow_default_constructible<
    cms::log::RuntimeAnsiFormatter>::value,
    "RuntimeAnsiFormatter construction must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<const cms::log::RuntimeAnsiFormatter&>().format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    cms::WriteResult>::value,
    "RuntimeAnsiFormatter must return WriteResult");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeAnsiFormatter&>().format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    "RuntimeAnsiFormatter format must preserve noexcept");
static_assert(noexcept(
    std::declval<cms::log::RuntimeAnsiFormatter&>().setUseColor(true)),
    "RuntimeAnsiFormatter setUseColor must preserve noexcept");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeAnsiFormatter&>().useColor()),
    "RuntimeAnsiFormatter useColor must preserve noexcept");
static_assert(std::is_empty<cms::log::StyledAnsiFormatter>::value,
    "StyledAnsiFormatter must remain stateless");
static_assert(std::is_same<
    decltype(cms::log::StyledAnsiFormatter::format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    cms::WriteResult>::value,
    "StyledAnsiFormatter must return WriteResult");
static_assert(noexcept(cms::log::StyledAnsiFormatter::format(
    std::declval<const cms::log::Record&>(),
    cms::StringBuffer())),
    "StyledAnsiFormatter must preserve noexcept");
static_assert(std::is_nothrow_default_constructible<
    cms::log::RuntimeStyledAnsiFormatter>::value,
    "RuntimeStyledAnsiFormatter construction must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<const cms::log::RuntimeStyledAnsiFormatter&>()
        .format(
            std::declval<const cms::log::Record&>(),
            cms::StringBuffer())),
    cms::WriteResult>::value,
    "RuntimeStyledAnsiFormatter must return WriteResult");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeStyledAnsiFormatter&>().format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    "RuntimeStyledAnsiFormatter format must preserve noexcept");
static_assert(noexcept(
    std::declval<cms::log::RuntimeStyledAnsiFormatter&>().setUseColor(true)),
    "RuntimeStyledAnsiFormatter setUseColor must preserve noexcept");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeStyledAnsiFormatter&>().useColor()),
    "RuntimeStyledAnsiFormatter useColor must preserve noexcept");
static_assert(std::is_empty<cms::log::NoLevelFilter>::value,
    "NoLevelFilter must remain stateless");
static_assert(std::is_nothrow_default_constructible<
    cms::log::RuntimeLevelFilter>::value,
    "RuntimeLevelFilter construction must preserve noexcept");
static_assert(std::is_same<
    decltype(cms::log::NoLevelFilter::allows(cms::log::Level::info)),
    bool>::value, "NoLevelFilter allows has the wrong return type");
static_assert(noexcept(
    cms::log::NoLevelFilter::allows(cms::log::Level::info)),
    "NoLevelFilter allows must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<cms::log::RuntimeLevelFilter&>().setMinLevel(
        cms::log::Level::warning)),
    void>::value, "RuntimeLevelFilter setMinLevel has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const cms::log::RuntimeLevelFilter&>().minLevel()),
    cms::log::Level>::value,
    "RuntimeLevelFilter minLevel has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const cms::log::RuntimeLevelFilter&>().allows(
        cms::log::Level::warning)),
    bool>::value, "RuntimeLevelFilter allows has the wrong return type");
static_assert(noexcept(
    std::declval<cms::log::RuntimeLevelFilter&>().setMinLevel(
        cms::log::Level::warning)),
    "RuntimeLevelFilter setMinLevel must preserve noexcept");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeLevelFilter&>().minLevel()),
    "RuntimeLevelFilter minLevel must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<cms::log::RuntimeLevelFilter&>().setEnabled(true)),
    void>::value, "RuntimeLevelFilter setEnabled has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const cms::log::RuntimeLevelFilter&>().enabled()),
    bool>::value, "RuntimeLevelFilter enabled has the wrong return type");
static_assert(noexcept(
    std::declval<cms::log::RuntimeLevelFilter&>().setEnabled(true)),
    "RuntimeLevelFilter setEnabled must preserve noexcept");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeLevelFilter&>().enabled()),
    "RuntimeLevelFilter enabled must preserve noexcept");
static_assert(noexcept(
    std::declval<const cms::log::RuntimeLevelFilter&>().allows(
        cms::log::Level::warning)),
    "RuntimeLevelFilter allows must preserve noexcept");

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
static_assert(std::is_constructible<
    NonMovableRuntimeLevelLogger, TestClock, TestSink>::value,
    "runtime filter must support a non-movable mutex");
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
static_assert(std::is_default_constructible<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must be constructible");
static_assert(std::is_default_constructible<StyledLogger>::value,
    "styled logger must be constructible");
static_assert(std::is_default_constructible<RuntimeStyledLogger>::value,
    "runtime styled logger must be constructible");
static_assert(std::is_default_constructible<
    RuntimeLevelRuntimeStyledLogger>::value,
    "runtime styled and level logger must be constructible");
static_assert(std::is_default_constructible<StdMutexRuntimeLogger>::value,
    "StdMutex runtime ANSI logger must be constructible");
static_assert(std::is_default_constructible<StdMutexRuntimeLevelLogger>::value,
    "StdMutex runtime level logger must be constructible");
static_assert(std::is_default_constructible<RuntimeLevelPlainLogger>::value,
    "plain runtime level logger must be constructible");
static_assert(std::is_default_constructible<RuntimeLevelAnsiLogger>::value,
    "ANSI runtime level logger must be constructible");
static_assert(std::is_default_constructible<
    RuntimeLevelRuntimeAnsiLogger>::value,
    "runtime ANSI and level logger must be constructible");
static_assert(!std::is_copy_constructible<RuntimeAnsiLogger>::value,
    "runtime ANSI logger copy must be deleted");
static_assert(!std::is_copy_assignable<RuntimeAnsiLogger>::value,
    "runtime ANSI logger copy assignment must be deleted");
static_assert(!std::is_move_constructible<RuntimeAnsiLogger>::value,
    "runtime ANSI logger move must be deleted");
static_assert(!std::is_move_assignable<RuntimeAnsiLogger>::value,
    "runtime ANSI logger move assignment must be deleted");
static_assert(!std::is_copy_constructible<RuntimeStyledLogger>::value,
    "runtime styled logger copy must be deleted");
static_assert(!std::is_copy_assignable<RuntimeStyledLogger>::value,
    "runtime styled logger copy assignment must be deleted");
static_assert(!std::is_move_constructible<RuntimeStyledLogger>::value,
    "runtime styled logger move must be deleted");
static_assert(!std::is_move_assignable<RuntimeStyledLogger>::value,
    "runtime styled logger move assignment must be deleted");
static_assert(!std::is_copy_constructible<RuntimeLevelPlainLogger>::value,
    "runtime level logger copy must be deleted");
static_assert(!std::is_copy_assignable<RuntimeLevelPlainLogger>::value,
    "runtime level logger copy assignment must be deleted");
static_assert(!std::is_move_constructible<RuntimeLevelPlainLogger>::value,
    "runtime level logger move must be deleted");
static_assert(!std::is_move_assignable<RuntimeLevelPlainLogger>::value,
    "runtime level logger move assignment must be deleted");

static_assert(std::is_same<
    decltype(std::declval<Logger&>().log(
        cms::log::Level::info, cms::StringView())),
    cms::Status>::value, "log has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().wouldLog(
        cms::log::Level::info)),
    bool>::value, "wouldLog has the wrong return type");
static_assert(noexcept(std::declval<const Logger&>().wouldLog(
    cms::log::Level::info)),
    "wouldLog must preserve the filter noexcept contract");
static_assert(std::is_same<
    decltype(std::declval<const RuntimeLevelPlainLogger&>().wouldLog(
        cms::log::Level::warning)),
    bool>::value, "runtime-filter wouldLog has the wrong return type");
static_assert(noexcept(
    std::declval<const RuntimeLevelPlainLogger&>().wouldLog(
        cms::log::Level::warning)),
    "runtime-filter wouldLog must preserve noexcept");
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
static_assert(!HasSetUseColor<Logger>::value,
    "plain logger must not expose runtime color state");
static_assert(!HasUseColor<Logger>::value,
    "plain logger must not expose runtime color state");
static_assert(!HasSetUseColor<AnsiLogger>::value,
    "always-ANSI logger must not expose runtime color state");
static_assert(!HasUseColor<AnsiLogger>::value,
    "always-ANSI logger must not expose runtime color state");
static_assert(!HasSetUseColor<StyledLogger>::value,
    "always-styled logger must not expose runtime color state");
static_assert(!HasUseColor<StyledLogger>::value,
    "always-styled logger must not expose runtime color state");
static_assert(HasSetUseColor<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must expose setUseColor");
static_assert(HasUseColor<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must expose useColor");
static_assert(std::is_same<
    decltype(std::declval<RuntimeAnsiLogger&>().setUseColor(true)),
    void>::value, "setUseColor has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const RuntimeAnsiLogger&>().useColor()),
    bool>::value, "useColor has the wrong return type");
static_assert(noexcept(
    std::declval<RuntimeAnsiLogger&>().setUseColor(true)),
    "runtime logger setUseColor must preserve noexcept");
static_assert(noexcept(
    std::declval<const RuntimeAnsiLogger&>().useColor()),
    "runtime logger useColor must preserve noexcept");
static_assert(HasSetUseColor<RuntimeStyledLogger>::value,
    "runtime styled logger must expose setUseColor");
static_assert(HasUseColor<RuntimeStyledLogger>::value,
    "runtime styled logger must expose useColor");
static_assert(noexcept(
    std::declval<RuntimeStyledLogger&>().setUseColor(true)),
    "runtime styled logger setUseColor must preserve noexcept");
static_assert(noexcept(
    std::declval<const RuntimeStyledLogger&>().useColor()),
    "runtime styled logger useColor must preserve noexcept");
static_assert(!HasSetMinLevel<Logger>::value,
    "no-filter logger must not expose runtime level state");
static_assert(!HasMinLevel<Logger>::value,
    "no-filter logger must not expose runtime level state");
static_assert(!HasSetLoggingEnabled<Logger>::value,
    "no-filter logger must not expose logging enabled state");
static_assert(!HasLoggingEnabled<Logger>::value,
    "no-filter logger must not expose logging enabled state");
static_assert(!HasSetMinLevel<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must not gain runtime level state by default");
static_assert(!HasMinLevel<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must not gain runtime level state by default");
static_assert(!HasSetLoggingEnabled<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must not gain logging enabled state by default");
static_assert(!HasLoggingEnabled<RuntimeAnsiLogger>::value,
    "runtime ANSI logger must not gain logging enabled state by default");
static_assert(HasSetMinLevel<RuntimeLevelPlainLogger>::value,
    "runtime level logger must expose setMinLevel");
static_assert(HasMinLevel<RuntimeLevelPlainLogger>::value,
    "runtime level logger must expose minLevel");
static_assert(HasSetLoggingEnabled<RuntimeLevelPlainLogger>::value,
    "runtime level logger must expose setLoggingEnabled");
static_assert(HasLoggingEnabled<RuntimeLevelPlainLogger>::value,
    "runtime level logger must expose loggingEnabled");
static_assert(std::is_same<
    decltype(std::declval<RuntimeLevelPlainLogger&>().setMinLevel(
        cms::log::Level::warning)),
    void>::value, "logger setMinLevel has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const RuntimeLevelPlainLogger&>().minLevel()),
    cms::log::Level>::value, "logger minLevel has the wrong return type");
static_assert(noexcept(
    std::declval<RuntimeLevelPlainLogger&>().setMinLevel(
        cms::log::Level::warning)),
    "logger setMinLevel must preserve noexcept");
static_assert(noexcept(
    std::declval<const RuntimeLevelPlainLogger&>().minLevel()),
    "logger minLevel must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<RuntimeLevelPlainLogger&>().setLoggingEnabled(true)),
    void>::value, "logger setLoggingEnabled has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const RuntimeLevelPlainLogger&>().loggingEnabled()),
    bool>::value, "logger loggingEnabled has the wrong return type");
static_assert(noexcept(
    std::declval<RuntimeLevelPlainLogger&>().setLoggingEnabled(true)),
    "logger setLoggingEnabled must preserve noexcept");
static_assert(noexcept(
    std::declval<const RuntimeLevelPlainLogger&>().loggingEnabled()),
    "logger loggingEnabled must preserve noexcept");
static_assert(HasSetUseColor<RuntimeLevelRuntimeAnsiLogger>::value,
    "combined runtime logger must expose setUseColor");
static_assert(HasSetMinLevel<RuntimeLevelRuntimeAnsiLogger>::value,
    "combined runtime logger must expose setMinLevel");
static_assert(HasSetLoggingEnabled<RuntimeLevelRuntimeAnsiLogger>::value,
    "combined runtime logger must expose setLoggingEnabled");
static_assert(HasSetUseColor<RuntimeLevelRuntimeStyledLogger>::value,
    "combined runtime styled logger must expose setUseColor");
static_assert(HasSetMinLevel<RuntimeLevelRuntimeStyledLogger>::value,
    "combined runtime styled logger must expose setMinLevel");
static_assert(HasSetLoggingEnabled<RuntimeLevelRuntimeStyledLogger>::value,
    "combined runtime styled logger must expose setLoggingEnabled");

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
static_assert(!HasFormatterAccessor<Logger>::value,
    "raw Formatter access must not escape the logger");
static_assert(!HasFormatterAccessor<RuntimeAnsiLogger>::value,
    "raw runtime Formatter access must not escape the logger");
static_assert(!HasFormatterAccessor<RuntimeStyledLogger>::value,
    "raw runtime styled Formatter access must not escape the logger");
static_assert(!HasLevelFilterAccessor<Logger>::value,
    "raw LevelFilter access must not escape the logger");
static_assert(!HasLevelFilterAccessor<RuntimeLevelPlainLogger>::value,
    "raw runtime LevelFilter access must not escape the logger");
