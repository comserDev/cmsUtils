#include <cms/log/level_filter.h>

int main() {
    cms::log::RuntimeLevelFilter filter;
    return filter.allows(cms::log::Level::debug) ? 0 : 1;
}
