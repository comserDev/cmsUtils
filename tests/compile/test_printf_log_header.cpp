#include <type_traits>

#include <cms/util/log/printf_log.h>
#include <cms/util/sync/null_mutex.h>

namespace {

struct HeaderClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct HeaderSink {
    void write(cms::util::StringView) noexcept {}
};

using HeaderLogger = cms::util::log::AsyncLogger<
    16,
    2,
    HeaderClock,
    HeaderSink,
    cms::util::sync::NullMutex>;
using OverwriteHeaderLogger = cms::util::log::AsyncLogger<
    16,
    2,
    HeaderClock,
    HeaderSink,
    cms::util::sync::NullMutex,
    cms::util::log::PlainFormatter,
    cms::util::log::NoLevelFilter,
    cms::util::log::OverwriteOldestOnFull>;

static_assert(std::is_same<
    decltype(cms::util::log::logf(
        std::declval<HeaderLogger&>(),
        cms::util::log::Level::info,
        "%d",
        1)),
    cms::util::Status>::value,
    "printf_log.h must expose Status-returning logf");
static_assert(std::is_same<
    decltype(cms::util::log::logf(
        std::declval<OverwriteHeaderLogger&>(),
        cms::util::log::Level::info,
        "%d",
        1)),
    cms::util::Status>::value,
    "logf must support an explicit overwrite logger");

} // namespace
