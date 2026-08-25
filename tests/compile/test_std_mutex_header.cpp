#include <cms/platform/std_mutex.h>

static_assert(
    sizeof(cms::platform::StdMutex) > 0,
    "std_mutex.h must compile independently");
