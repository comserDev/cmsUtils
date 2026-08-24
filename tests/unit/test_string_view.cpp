#include <cstdio>

#include <cms/string_view.h>

#include "test.h"

int main() {
    const cms::StringView empty;
    CMS_TEST_CHECK(empty.data() == nullptr);
    CMS_TEST_CHECK(empty.size() == 0);
    CMS_TEST_CHECK(empty.empty());

    const cms::StringView literal("hello");
    CMS_TEST_REQUIRE(literal.data() != nullptr);
    CMS_TEST_CHECK(literal.size() == 5);
    CMS_TEST_CHECK(!literal.empty());
    CMS_TEST_CHECK(literal[0] == 'h');
    CMS_TEST_CHECK(literal[4] == 'o');

    char padded[8] = "abc";
    const cms::StringView paddedArray(padded);
    CMS_TEST_REQUIRE(paddedArray.data() == padded);
    CMS_TEST_CHECK(paddedArray.size() == 3);

    const char raw[] = {'r', 'a', 'w'};
    const cms::StringView rawArray(raw);
    CMS_TEST_REQUIRE(rawArray.data() == raw);
    CMS_TEST_CHECK(rawArray.size() == 3);

    const cms::StringView explicitLength(raw, sizeof(raw));
    CMS_TEST_REQUIRE(explicitLength.data() != nullptr);
    CMS_TEST_CHECK(explicitLength.size() == 3);
    CMS_TEST_CHECK(explicitLength[2] == 'w');

    const char embedded[] = {'A', '\0', 'B'};
    const cms::StringView embeddedArray(embedded);
    CMS_TEST_REQUIRE(embeddedArray.data() == embedded);
    CMS_TEST_CHECK(embeddedArray.size() == 1);

    const cms::StringView embeddedNul(embedded, sizeof(embedded));
    CMS_TEST_CHECK(embeddedNul.size() == 3);
    CMS_TEST_CHECK(embeddedNul[0] == 'A');
    CMS_TEST_CHECK(embeddedNul[1] == '\0');
    CMS_TEST_CHECK(embeddedNul[2] == 'B');

    const cms::StringView middle = literal.substr(1, 3);
    CMS_TEST_REQUIRE(middle.data() != nullptr);
    CMS_TEST_CHECK(middle.size() == 3);
    CMS_TEST_CHECK(middle[0] == 'e');
    CMS_TEST_CHECK(middle[2] == 'l');

    const cms::StringView clipped = literal.substr(3, 100);
    CMS_TEST_REQUIRE(clipped.data() != nullptr);
    CMS_TEST_CHECK(clipped.size() == 2);
    CMS_TEST_CHECK(clipped[0] == 'l');
    CMS_TEST_CHECK(clipped[1] == 'o');

    const cms::StringView pastEnd = literal.substr(6, 1);
    CMS_TEST_CHECK(pastEnd.data() == nullptr);
    CMS_TEST_CHECK(pastEnd.empty());

    const cms::StringView copied = literal;
    CMS_TEST_CHECK(copied.data() == literal.data());
    CMS_TEST_CHECK(copied.size() == literal.size());

    const cms::StringView nullEmpty(nullptr, 0);
    CMS_TEST_CHECK(nullEmpty.data() == nullptr);
    CMS_TEST_CHECK(nullEmpty.empty());

    const cms::StringView normalizedInvalid(nullptr, 5);
    CMS_TEST_CHECK(normalizedInvalid.data() == nullptr);
    CMS_TEST_CHECK(normalizedInvalid.size() == 0);
    CMS_TEST_CHECK(normalizedInvalid.empty());

    std::printf("sizeof(cms::StringView)=%zu\n", sizeof(cms::StringView));
    return cms::test::finish();
}
