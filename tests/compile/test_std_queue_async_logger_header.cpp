#include <type_traits>

#include <cms/util/log/std_queue_async_logger.h>
#include <cms/util/sync/null_mutex.h>

namespace {

struct HeaderClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct HeaderSink {
    void write(cms::util::StringView) noexcept {}
};

using HeaderLogger = cms::util::log::StdQueueAsyncLogger<
    16,
    HeaderClock,
    HeaderSink,
    cms::util::sync::NullMutex>;

static_assert(
    HeaderLogger::messageCapacity() == 16,
    "std queue logger header must expose message capacity");
static_assert(
    std::is_default_constructible<HeaderLogger>::value,
    "std queue logger header must be independently usable");

} // namespace
