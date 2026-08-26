#include <cms/util/log/sink.h>

struct HeaderSink {
    void write(cms::util::StringView) noexcept {}
};

static_assert(sizeof(HeaderSink) > 0, "sink.h must compile independently");
