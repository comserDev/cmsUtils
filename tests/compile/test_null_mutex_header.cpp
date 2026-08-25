#include <cms/sync/null_mutex.h>

static_assert(
    sizeof(cms::sync::NullMutex) > 0,
    "null_mutex.h must compile independently");
