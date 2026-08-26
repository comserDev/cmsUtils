#include <cms/util/platform/steady_clock.h>

static_assert(
    sizeof(cms::util::platform::SteadyClock) > 0,
    "steady_clock.h must compile independently");
