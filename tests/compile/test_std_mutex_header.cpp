#include <cms/util/platform/std_mutex.h>

static_assert(
    sizeof(cms::util::platform::StdMutex) > 0,
    "std_mutex.h must compile independently");
