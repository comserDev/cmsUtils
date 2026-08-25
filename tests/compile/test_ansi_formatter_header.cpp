#include <cms/log/ansi_formatter.h>

static_assert(
    cms::log::AnsiFormatter::maxOverhead
        == cms::log::maxAnsiFormattedRecordOverhead,
    "ansi_formatter.h must compile independently");
