#include <cms/platform/system_clock.h>

static_assert(
    sizeof(cms::platform::SystemClock) > 0,
    "system_clock.h must compile independently");
