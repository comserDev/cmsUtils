#include <cstdio>

#include <cms/util/status.h>

#include "test.h"

int main() {
    CMS_TEST_CHECK(cms::util::Status::io_error
        != cms::util::Status::no_space);
    const cms::util::WriteResult success{
        cms::util::Status::ok,
        5,
        5
    };

    CMS_TEST_CHECK(success.status == cms::util::Status::ok);
    CMS_TEST_CHECK(success.written == 5);
    CMS_TEST_CHECK(success.required == 5);

    const cms::util::WriteResult noSpace{
        cms::util::Status::no_space,
        0,
        10
    };

    CMS_TEST_CHECK(noSpace.status == cms::util::Status::no_space);
    CMS_TEST_CHECK(noSpace.written == 0);
    CMS_TEST_CHECK(noSpace.required == 10);

    const cms::util::WriteResult truncated{
        cms::util::Status::no_space,
        4,
        10
    };

    CMS_TEST_CHECK(truncated.status == cms::util::Status::no_space);
    CMS_TEST_CHECK(truncated.written == 4);
    CMS_TEST_CHECK(truncated.required == 10);

    const cms::util::ParseResult<int> parsed{
        cms::util::Status::ok,
        123,
        3
    };

    CMS_TEST_CHECK(parsed.status == cms::util::Status::ok);
    CMS_TEST_CHECK(parsed.value == 123);
    CMS_TEST_CHECK(parsed.consumed == 3);

    const cms::util::ParseResult<int> failed{
        cms::util::Status::invalid_argument
    };

    CMS_TEST_CHECK(failed.status == cms::util::Status::invalid_argument);
    CMS_TEST_CHECK(failed.value == 0);
    CMS_TEST_CHECK(failed.consumed == 0);

    std::printf("sizeof(cms::util::Status)=%zu\n", sizeof(cms::util::Status));
    std::printf("sizeof(cms::util::WriteResult)=%zu\n", sizeof(cms::util::WriteResult));
    std::printf(
        "sizeof(cms::util::ParseResult<int>)=%zu\n",
        sizeof(cms::util::ParseResult<int>));

    return cms::test::finish();
}
