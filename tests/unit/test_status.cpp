#include <cstdio>

#include <cms/status.h>

#include "test.h"

int main() {
    const cms::WriteResult success{
        cms::Status::ok,
        5,
        5
    };

    CMS_TEST_CHECK(success.status == cms::Status::ok);
    CMS_TEST_CHECK(success.written == 5);
    CMS_TEST_CHECK(success.required == 5);

    const cms::WriteResult noSpace{
        cms::Status::no_space,
        0,
        10
    };

    CMS_TEST_CHECK(noSpace.status == cms::Status::no_space);
    CMS_TEST_CHECK(noSpace.written == 0);
    CMS_TEST_CHECK(noSpace.required == 10);

    const cms::WriteResult truncated{
        cms::Status::no_space,
        4,
        10
    };

    CMS_TEST_CHECK(truncated.status == cms::Status::no_space);
    CMS_TEST_CHECK(truncated.written == 4);
    CMS_TEST_CHECK(truncated.required == 10);

    const cms::ParseResult<int> parsed{
        cms::Status::ok,
        123,
        3
    };

    CMS_TEST_CHECK(parsed.status == cms::Status::ok);
    CMS_TEST_CHECK(parsed.value == 123);
    CMS_TEST_CHECK(parsed.consumed == 3);

    const cms::ParseResult<int> failed{
        cms::Status::invalid_argument
    };

    CMS_TEST_CHECK(failed.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(failed.value == 0);
    CMS_TEST_CHECK(failed.consumed == 0);

    std::printf("sizeof(cms::Status)=%zu\n", sizeof(cms::Status));
    std::printf("sizeof(cms::WriteResult)=%zu\n", sizeof(cms::WriteResult));
    std::printf(
        "sizeof(cms::ParseResult<int>)=%zu\n",
        sizeof(cms::ParseResult<int>));

    return cms::test::finish();
}
