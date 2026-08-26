#include <cms/util/sync/null_mutex.h>

static_assert(
    sizeof(cms::util::sync::NullMutex) > 0,
    "null_mutex.h must compile independently");
