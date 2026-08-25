#include <cms/log/sink.h>

struct HeaderSink {
    void write(cms::StringView) noexcept {}
};

static_assert(sizeof(HeaderSink) > 0, "sink.h must compile independently");
