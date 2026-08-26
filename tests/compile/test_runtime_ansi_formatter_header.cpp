#include <cms/util/log/runtime_ansi_formatter.h>

int main() {
    cms::util::log::RuntimeAnsiFormatter formatter;
    return formatter.useColor() ? 0 : 1;
}
