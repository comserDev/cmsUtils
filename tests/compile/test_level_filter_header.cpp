#include <cms/util/log/level_filter.h>

int main() {
    cms::util::log::RuntimeLevelFilter filter;
    return filter.allows(cms::util::log::Level::debug) ? 0 : 1;
}
