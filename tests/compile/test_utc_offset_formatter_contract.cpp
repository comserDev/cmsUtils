#include <type_traits>
#include <utility>

#include <cms/log/ansi_formatter.h>
#include <cms/log/async_logger.h>
#include <cms/log/level_filter.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/log/utc_offset_formatter.h>
#include <cms/platform/system_clock.h>
#include <cms/sync/null_mutex.h>

namespace {

struct TestClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct TestSink {
    void write(cms::StringView) noexcept {}
};

struct UnsupportedFormatter {};

using UtcPlain = cms::log::UtcOffsetFormatter<>;
using UtcAnsi = cms::log::UtcOffsetFormatter<cms::log::AnsiFormatter>;
using UtcRuntimeAnsi =
    cms::log::UtcOffsetFormatter<cms::log::RuntimeAnsiFormatter>;
using UtcStyled =
    cms::log::UtcOffsetFormatter<cms::log::StyledAnsiFormatter>;
using UtcRuntimeStyled =
    cms::log::UtcOffsetFormatter<cms::log::RuntimeStyledAnsiFormatter>;
using Logger = cms::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    UtcPlain>;
using RuntimeLogger = cms::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::sync::NullMutex,
    UtcRuntimeStyled,
    cms::log::RuntimeLevelFilter>;

} // namespace

static_assert(cms::log::detail::IsSupportedUtcOffsetBase<
    cms::log::PlainFormatter>::value,
    "plain formatter must remain supported");
static_assert(cms::log::detail::IsSupportedUtcOffsetBase<
    cms::log::AnsiFormatter>::value,
    "ANSI formatter must remain supported");
static_assert(cms::log::detail::IsSupportedUtcOffsetBase<
    cms::log::RuntimeAnsiFormatter>::value,
    "runtime ANSI formatter must remain supported");
static_assert(cms::log::detail::IsSupportedUtcOffsetBase<
    cms::log::StyledAnsiFormatter>::value,
    "styled formatter must remain supported");
static_assert(cms::log::detail::IsSupportedUtcOffsetBase<
    cms::log::RuntimeStyledAnsiFormatter>::value,
    "runtime styled formatter must remain supported");
static_assert(!cms::log::detail::IsSupportedUtcOffsetBase<
    UnsupportedFormatter>::value,
    "arbitrary formatter must not satisfy the built-in contract");

static_assert(cms::log::minUtcOffsetMinutes == -720,
    "minimum UTC offset changed");
static_assert(cms::log::maxUtcOffsetMinutes == 840,
    "maximum UTC offset changed");
static_assert(cms::log::utcOffsetTimestampSize == 8,
    "UTC offset timestamp payload size changed");
static_assert(std::is_same<
    decltype(cms::log::formatUtcOffsetTimestamp(
        cms::log::Timestamp{0}, 0, cms::StringBuffer())),
    cms::WriteResult>::value,
    "formatUtcOffsetTimestamp must return WriteResult");
static_assert(noexcept(cms::log::formatUtcOffsetTimestamp(
    cms::log::Timestamp{0}, 0, cms::StringBuffer())),
    "formatUtcOffsetTimestamp must preserve noexcept");
static_assert(std::is_nothrow_default_constructible<UtcPlain>::value,
    "UTC formatter must be nothrow default constructible");
static_assert(std::is_nothrow_default_constructible<UtcRuntimeAnsi>::value,
    "runtime ANSI UTC formatter must be nothrow default constructible");
static_assert(std::is_nothrow_default_constructible<UtcStyled>::value,
    "styled UTC formatter must be nothrow default constructible");
static_assert(std::is_nothrow_default_constructible<UtcRuntimeStyled>::value,
    "runtime styled UTC formatter must be nothrow default constructible");
static_assert(std::is_same<
    decltype(std::declval<UtcAnsi&>().setUtcOffsetMinutes(0)),
    cms::Status>::value,
    "UTC offset setter must return Status");
static_assert(std::is_same<
    decltype(std::declval<const UtcAnsi&>().utcOffsetMinutes()),
    int>::value,
    "UTC offset getter must return int");
static_assert(noexcept(std::declval<UtcAnsi&>().setUtcOffsetMinutes(0)),
    "UTC offset setter must preserve noexcept");
static_assert(noexcept(std::declval<const UtcAnsi&>().utcOffsetMinutes()),
    "UTC offset getter must preserve noexcept");
static_assert(std::is_same<
    decltype(std::declval<const UtcPlain&>().format(
        std::declval<const cms::log::Record&>(),
        cms::StringBuffer())),
    cms::WriteResult>::value,
    "UTC formatter must return WriteResult");
static_assert(noexcept(std::declval<const UtcPlain&>().format(
    std::declval<const cms::log::Record&>(),
    cms::StringBuffer())),
    "UTC formatter must preserve noexcept");
static_assert(std::is_default_constructible<Logger>::value,
    "UTC formatter must integrate with AsyncLogger");
static_assert(std::is_default_constructible<RuntimeLogger>::value,
    "runtime styled UTC formatter must integrate with AsyncLogger");
static_assert(std::is_same<
    decltype(std::declval<Logger&>().setUtcOffsetMinutes(0)),
    cms::Status>::value,
    "UTC offset logger setter must return Status");
static_assert(std::is_same<
    decltype(std::declval<const Logger&>().utcOffsetMinutes()),
    int>::value,
    "UTC offset logger getter must return int");
static_assert(noexcept(std::declval<Logger&>().setUtcOffsetMinutes(0)),
    "UTC offset logger setter must preserve noexcept");
static_assert(noexcept(std::declval<const Logger&>().utcOffsetMinutes()),
    "UTC offset logger getter must preserve noexcept");
static_assert(!std::is_copy_constructible<RuntimeLogger>::value,
    "UTC logger copy must remain deleted");
static_assert(!std::is_move_constructible<RuntimeLogger>::value,
    "UTC logger move must remain deleted");
