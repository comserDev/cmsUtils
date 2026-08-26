#include <cms/util/log/clock.h>

static_assert(
    sizeof(cms::util::log::Timestamp) > 0,
    "clock.h must compile independently");
