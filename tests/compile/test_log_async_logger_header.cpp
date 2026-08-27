#include <cms/util/log/async_logger.h>

struct HeaderClock {
    cms::util::log::Timestamp nowMilliseconds() noexcept { return 0; }
};

struct HeaderSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
};

struct HeaderMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

using HeaderLogger = cms::util::log::AsyncLogger<
    8, 2, HeaderClock, HeaderSink, HeaderMutex>;

static_assert(
    sizeof(HeaderLogger) > 0,
    "async_logger.h must compile independently");
