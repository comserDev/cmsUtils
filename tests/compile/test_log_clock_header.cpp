#include <cms/log/clock.h>

static_assert(
    sizeof(cms::log::Timestamp) > 0,
    "clock.h must compile independently");
