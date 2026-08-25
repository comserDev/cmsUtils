#include <cms/log/styled_ansi_formatter.h>

static_assert(
    cms::log::maxStyledMessageExpansionFactor == 4,
    "styled formatter bound changed");
static_assert(
    sizeof(cms::log::StyledAnsiFormatter) > 0,
    "styled_ansi_formatter.h must compile independently");

int main() {
    cms::log::RuntimeStyledAnsiFormatter formatter;
    return formatter.useColor() ? 0 : 1;
}
