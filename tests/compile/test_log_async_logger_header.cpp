#include <cms/log/async_logger.h>

struct HeaderClock {
    cms::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct HeaderSink {
    void write(cms::StringView) noexcept {}
};

struct HeaderMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

using HeaderLogger = cms::log::AsyncLogger<
    8, 2, HeaderClock, HeaderSink, HeaderMutex>;

static_assert(
    sizeof(HeaderLogger) > 0,
    "async_logger.h must compile independently");
