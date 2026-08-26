#include <cms/util/static_queue.h>

static_assert(
    sizeof(cms::util::StaticQueue<int, 1>) > 0,
    "static_queue.h must compile independently");
