#include <cms/util/log/formatter.h>

static_assert(
    cms::util::log::maxFormattedRecordOverhead == 35,
    "formatter.h must compile independently");
