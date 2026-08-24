#include <cstring>

#include "test.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }

    if (std::strcmp(argv[1], "success") == 0) {
        CMS_TEST_CHECK(true);
        return cms::test::finish();
    }

    if (std::strcmp(argv[1], "check_failure") == 0) {
        CMS_TEST_CHECK(false);
        return cms::test::finish();
    }

    if (std::strcmp(argv[1], "require_failure") == 0) {
        CMS_TEST_REQUIRE(false);
        return cms::test::finish();
    }

    if (std::strcmp(argv[1], "zero") == 0) {
        return cms::test::finish();
    }

    return 2;
}
