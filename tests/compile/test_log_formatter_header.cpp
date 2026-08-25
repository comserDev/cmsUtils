#include <cms/log/formatter.h>

static_assert(
    cms::log::maxFormattedRecordOverhead == 35,
    "formatter.h must compile independently");
