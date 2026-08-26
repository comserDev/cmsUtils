#include <type_traits>

#include <cms/log/printf_log.h>
#include <cms/sync/null_mutex.h>

namespace {

struct HeaderClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct HeaderSink {
    void write(cms::StringView) noexcept {}
};

using HeaderLogger = cms::log::AsyncLogger<
    16,
    2,
    HeaderClock,
    HeaderSink,
    cms::sync::NullMutex>;

static_assert(std::is_same<
    decltype(cms::log::logf(
        std::declval<HeaderLogger&>(),
        cms::log::Level::info,
        "%d",
        1)),
    cms::Status>::value,
    "printf_log.h must expose Status-returning logf");

} // namespace
