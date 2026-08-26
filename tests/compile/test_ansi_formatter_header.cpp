#include <cms/util/log/ansi_formatter.h>

static_assert(
    cms::util::log::AnsiFormatter::maxOverhead
        == cms::util::log::maxAnsiFormattedRecordOverhead,
    "ansi_formatter.h must compile independently");
