#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <cms/util/log/level_filter.h>

#include "test.h"

int main() {
    cms::util::log::NoLevelFilter noFilter;
    const cms::util::log::Level invalid = static_cast<cms::util::log::Level>(0xFF);
    const cms::util::log::Level levels[] = {
        cms::util::log::Level::trace,
        cms::util::log::Level::debug,
        cms::util::log::Level::info,
        cms::util::log::Level::warning,
        cms::util::log::Level::error,
        cms::util::log::Level::critical};

    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(noFilter.allows(levels[index]));
    }
    CMS_TEST_CHECK(noFilter.allows(invalid));

    cms::util::log::RuntimeLevelFilter filter;
    CMS_TEST_CHECK(filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::debug);
    CMS_TEST_CHECK(!filter.allows(cms::util::log::Level::trace));
    CMS_TEST_CHECK(filter.allows(cms::util::log::Level::debug));
    CMS_TEST_CHECK(filter.allows(cms::util::log::Level::info));
    CMS_TEST_CHECK(filter.allows(cms::util::log::Level::warning));
    CMS_TEST_CHECK(filter.allows(cms::util::log::Level::error));
    CMS_TEST_CHECK(filter.allows(cms::util::log::Level::critical));
    CMS_TEST_CHECK(filter.allows(invalid));

    filter.setMinLevel(cms::util::log::Level::warning);
    filter.setEnabled(false);
    CMS_TEST_CHECK(!filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::warning);
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(!filter.allows(levels[index]));
    }
    CMS_TEST_CHECK(!filter.allows(invalid));

    filter.setEnabled(true);
    CMS_TEST_CHECK(filter.enabled());
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::warning);
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

    filter.setMinLevel(cms::util::log::Level::critical);
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::critical);
    filter.setMinLevel(cms::util::log::Level::trace);
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::trace);
    filter.setMinLevel(cms::util::log::Level::warning);
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::warning);

    filter.setMinLevel(invalid);
    CMS_TEST_CHECK(filter.minLevel() == cms::util::log::Level::trace);
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        CMS_TEST_CHECK(filter.allows(levels[index]));
    }
    CMS_TEST_CHECK(filter.allows(invalid));

    std::printf(
        "sizeof(cms::util::log::NoLevelFilter)=%zu\n",
        sizeof(cms::util::log::NoLevelFilter));
    std::printf(
        "sizeof(cms::util::log::RuntimeLevelFilter)=%zu\n",
        sizeof(cms::util::log::RuntimeLevelFilter));

    return cms::test::finish();
}
