#include <cms/log/runtime_ansi_formatter.h>

int main() {
    cms::log::RuntimeAnsiFormatter formatter;
    return formatter.useColor() ? 0 : 1;
}
