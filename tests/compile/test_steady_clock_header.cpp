#include <cms/platform/steady_clock.h>

static_assert(
    sizeof(cms::platform::SteadyClock) > 0,
    "steady_clock.h must compile independently");
