#include <cms/util/platform/system_clock.h>

static_assert(
    sizeof(cms::util::platform::SystemClock) > 0,
    "system_clock.h must compile independently");
