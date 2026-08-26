#include <cms/util/log/record.h>

static_assert(
    sizeof(cms::util::log::StaticRecord<8>) > 0,
    "record.h must compile independently");
