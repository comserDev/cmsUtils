#include <type_traits>
#include <utility>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/log/level_filter.h>
#include <cms/util/log/runtime_ansi_formatter.h>
#include <cms/util/log/styled_ansi_formatter.h>
#include <cms/util/log/utc_offset_formatter.h>
#include <cms/util/platform/system_clock.h>
#include <cms/util/sync/null_mutex.h>

namespace {

struct TestClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct TestSink {
    void write(cms::util::StringView) noexcept {}
};

struct UnsupportedFormatter {};

using UtcPlain = cms::util::log::UtcOffsetFormatter<>;
using UtcAnsi = cms::util::log::UtcOffsetFormatter<cms::util::log::AnsiFormatter>;
using UtcRuntimeAnsi =
    cms::util::log::UtcOffsetFormatter<cms::util::log::RuntimeAnsiFormatter>;
using UtcStyled =
    cms::util::log::UtcOffsetFormatter<cms::util::log::StyledAnsiFormatter>;
using UtcRuntimeStyled =
    cms::util::log::UtcOffsetFormatter<cms::util::log::RuntimeStyledAnsiFormatter>;
using Logger = cms::util::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::util::sync::NullMutex,
    UtcPlain>;
using RuntimeLogger = cms::util::log::AsyncLogger<
    16,
    2,
    TestClock,
    TestSink,
    cms::util::sync::NullMutex,
    UtcRuntimeStyled,
    cms::util::log::RuntimeLevelFilter>;

} // namespace

static_assert(cms::util::log::detail::IsSupportedUtcOffsetBase<
    cms::util::log::PlainFormatter>::value,
    "plain formatter must remain supported");
static_assert(cms::util::log::detail::IsSupportedUtcOffsetBase<
    cms::util::log::AnsiFormatter>::value,
    "ANSI formatter must remain supported");
static_assert(cms::util::log::detail::IsSupportedUtcOffsetBase<
    cms::util::log::RuntimeAnsiFormatter>::value,
    "runtime ANSI formatter must remain supported");
static_assert(cms::util::log::detail::IsSupportedUtcOffsetBase<
    cms::util::log::StyledAnsiFormatter>::value,
    "styled formatter must remain supported");
static_assert(cms::util::log::detail::IsSupportedUtcOffsetBase<
    cms::util::log::RuntimeStyledAnsiFormatter>::value,
    "runtime styled formatter must remain supported");
static_assert(!cms::util::log::detail::IsSupportedUtcOffsetBase<
    UnsupportedFormatter>::value,
    "arbitrary formatter must not satisfy the built-in contract");

static_assert(cms::util::log::minUtcOffsetMinutes == -720,
    "minimum UTC offset changed");
static_assert(cms::util::log::maxUtcOffsetMinutes == 840,
    "maximum UTC offset changed");
static_assert(cms::util::log::utcOffsetTimestampSize == 8,
    "UTC offset timestamp payload size changed");
static_assert(std::is_same<
    decltype(cms::util::log::formatUtcOffsetTimestamp(
        cms::util::log::Timestamp{0}, 0, cms::util::StringBuffer())),
    cms::util::WriteResult>::value,
    "formatUtcOffsetTimestamp must return WriteResult");
static_assert(noexcept(cms::util::log::formatUtcOffsetTimestamp(
    cms::util::log::Timestamp{0}, 0, cms::util::StringBuffer())),
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
    cms::util::Status>::value,
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
        std::declval<const cms::util::log::Record&>(),
        cms::util::StringBuffer())),
    cms::util::WriteResult>::value,
    "UTC formatter must return WriteResult");
static_assert(noexcept(std::declval<const UtcPlain&>().format(
    std::declval<const cms::util::log::Record&>(),
    cms::util::StringBuffer())),
    "UTC formatter must preserve noexcept");
static_assert(std::is_default_constructible<Logger>::value,
    "UTC formatter must integrate with AsyncLogger");
static_assert(std::is_default_constructible<RuntimeLogger>::value,
    "runtime styled UTC formatter must integrate with AsyncLogger");
static_assert(std::is_same<
    decltype(std::declval<Logger&>().setUtcOffsetMinutes(0)),
    cms::util::Status>::value,
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
