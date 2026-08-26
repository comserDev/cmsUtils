#include <cms/util/log/level.h>

static_assert(
    static_cast<unsigned int>(cms::util::log::Level::trace) == 0,
    "level.h must compile independently");
