#include <cms/log/record.h>

static_assert(
    sizeof(cms::log::StaticRecord<8>) > 0,
    "record.h must compile independently");
