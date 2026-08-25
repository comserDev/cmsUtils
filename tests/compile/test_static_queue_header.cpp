#include <cms/static_queue.h>

static_assert(
    sizeof(cms::StaticQueue<int, 1>) > 0,
    "static_queue.h must compile independently");
