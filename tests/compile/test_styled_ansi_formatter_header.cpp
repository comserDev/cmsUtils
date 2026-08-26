#include <cms/util/log/styled_ansi_formatter.h>

static_assert(
    cms::util::log::maxStyledMessageExpansionFactor == 4,
    "styled formatter bound changed");
static_assert(
    sizeof(cms::util::log::StyledAnsiFormatter) > 0,
    "styled_ansi_formatter.h must compile independently");

int main() {
    cms::util::log::RuntimeStyledAnsiFormatter formatter;
    return formatter.useColor() ? 0 : 1;
}
