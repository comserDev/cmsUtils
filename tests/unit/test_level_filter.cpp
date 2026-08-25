#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <cms/log/level_filter.h>

#include "test.h"

int main() {
    cms::log::NoLevelFilter noFilter;
    const cms::log::Level invalid = static_cast<cms::log::Level>(0xFF);
    const cms::log::Level levels[] = {
        cms::log::Level::trace,
        cms::log::Level::debug,
        cms::log::Level::info,
        cms::log::Level::warning,
        cms::log::Level::error,
        cms::log::Level::critical};

    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(noFilter.allows(levels[index]));
    }
    CMS_TEST_CHECK(noFilter.allows(invalid));

    cms::log::RuntimeLevelFilter filter;
    CMS_TEST_CHECK(filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::debug);
    CMS_TEST_CHECK(!filter.allows(cms::log::Level::trace));
    CMS_TEST_CHECK(filter.allows(cms::log::Level::debug));
    CMS_TEST_CHECK(filter.allows(cms::log::Level::info));
    CMS_TEST_CHECK(filter.allows(cms::log::Level::warning));
    CMS_TEST_CHECK(filter.allows(cms::log::Level::error));
    CMS_TEST_CHECK(filter.allows(cms::log::Level::critical));
    CMS_TEST_CHECK(filter.allows(invalid));

    filter.setMinLevel(cms::log::Level::warning);
    filter.setEnabled(false);
    CMS_TEST_CHECK(!filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::warning);
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(!filter.allows(levels[index]));
    }
    CMS_TEST_CHECK(!filter.allows(invalid));

    filter.setEnabled(true);
    CMS_TEST_CHECK(filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::warning);
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(filter.allows(levels[index]) == (index >= 3));
    }
    CMS_TEST_CHECK(filter.allows(invalid));

    for (std::size_t minimum = 0;
         minimum < sizeof(levels) / sizeof(levels[0]);
         ++minimum) {
        filter.setMinLevel(levels[minimum]);
        CMS_TEST_CHECK(filter.minLevel() == levels[minimum]);
        for (std::size_t current = 0;
             current < sizeof(levels) / sizeof(levels[0]);
             ++current) {
            CMS_TEST_CHECK(filter.allows(levels[current])
                == (current >= minimum));
        }
        CMS_TEST_CHECK(filter.allows(invalid));
    }

    filter.setMinLevel(cms::log::Level::critical);
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::critical);
    filter.setMinLevel(cms::log::Level::trace);
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::trace);
    filter.setMinLevel(cms::log::Level::warning);
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::warning);

    filter.setMinLevel(invalid);
    CMS_TEST_CHECK(filter.minLevel() == cms::log::Level::trace);
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(filter.allows(levels[index]));
    }
    CMS_TEST_CHECK(filter.allows(invalid));

    std::printf(
        "sizeof(cms::log::NoLevelFilter)=%zu\n",
        sizeof(cms::log::NoLevelFilter));
    std::printf(
        "sizeof(cms::log::RuntimeLevelFilter)=%zu\n",
        sizeof(cms::log::RuntimeLevelFilter));

    return cms::test::finish();
}
