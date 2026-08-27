#include <cms/util/log/sink.h>

struct HeaderSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
};

static_assert(sizeof(HeaderSink) > 0, "sink.h must compile independently");
