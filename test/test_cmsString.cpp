#include <climits>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

#include "../src/cmsString.h"
#include "../src/cmsQueue.h"
#include "test.h"

template <size_t N>
static void assertInvariant(const cms::String<N>& value) {
    CMS_TEST_REQUIRE(value.c_str() != nullptr);
    CMS_TEST_REQUIRE(value.length() < value.capacity());
    CMS_TEST_REQUIRE(value.c_str()[value.length()] == '\0');
    CMS_TEST_CHECK(std::strlen(value.c_str()) == value.length());
}

static cms::String<32> returnString() {
    cms::String<32> value = "return";
    return value;
}

static void testCopyAndMove() {
    cms::String<32> a = "abc";
    cms::String<32> b(a);
    CMS_TEST_CHECK(a.c_str() != b.c_str());
    b += "X";
    CMS_TEST_CHECK(a == "abc");
    CMS_TEST_CHECK(b == "abcX");

    cms::String<32> c(std::move(b));
    CMS_TEST_CHECK(c.c_str() != b.c_str());
    CMS_TEST_CHECK(c == "abcX");
    CMS_TEST_CHECK(b == "");

    cms::String<32> d;
    d = a;
    CMS_TEST_CHECK(d.c_str() != a.c_str());
    d = std::move(c);
    CMS_TEST_CHECK(d.c_str() != c.c_str());
    CMS_TEST_CHECK(d == "abcX");
    CMS_TEST_CHECK(c == "");

    cms::String<32> sum = a + "Z";
    CMS_TEST_CHECK(sum.c_str() != a.c_str());
    CMS_TEST_CHECK(a == "abc");
    CMS_TEST_CHECK(sum == "abcZ");

    cms::String<32> returned = returnString();
    CMS_TEST_CHECK(returned == "return");
    assertInvariant(a);
    assertInvariant(b);
    assertInvariant(c);
    assertInvariant(d);
    assertInvariant(sum);
    assertInvariant(returned);
}

static void testPointerConstruction() {
    const char* text = "pointer";
    cms::String<16> value(text);
    cms::String<16> nullValue(static_cast<const char*>(nullptr));
    CMS_TEST_CHECK(value == "pointer");
    CMS_TEST_CHECK(nullValue == "");
    assertInvariant(value);
    assertInvariant(nullValue);
}

static void testTransactionalNumbers() {
    cms::String<4> integer = "A";
    integer.appendInt(-12);
    CMS_TEST_CHECK(integer == "A");
    assertInvariant(integer);

    cms::String<4> positive = "A";
    positive.appendInt(123);
    CMS_TEST_CHECK(positive == "A");
    assertInvariant(positive);

    cms::String<4> unsignedFailure = "A";
    unsignedFailure.appendUInt(123UL);
    CMS_TEST_CHECK(unsignedFailure == "A");
    assertInvariant(unsignedFailure);

    cms::String<5> floating = "X";
    floating.appendFloat(1.25f, 2);
    CMS_TEST_CHECK(floating == "X");
    assertInvariant(floating);

    cms::String<32> enough = "N=";
    enough.appendInt(-123);
    CMS_TEST_CHECK(enough == "N=-123");
    enough.appendFloat(1.25f, 2);
    CMS_TEST_CHECK(enough == "N=-1231.25");
    assertInvariant(enough);

    cms::String<64> limits;
    limits.appendInt(LONG_MIN);
#if LONG_MAX == 2147483647L
    CMS_TEST_CHECK(limits == "-2147483648");
#else
    CMS_TEST_CHECK(limits == "-9223372036854775808");
#endif
    limits.clear();
    limits.appendInt(LONG_MAX);
#if LONG_MAX == 2147483647L
    CMS_TEST_CHECK(limits == "2147483647");
#else
    CMS_TEST_CHECK(limits == "9223372036854775807");
#endif
    assertInvariant(limits);

    cms::String<64> negative;
    negative.appendFloat(-12.5f, 2);
    CMS_TEST_CHECK(negative == "-12.50");
    negative.clear();
    negative.appendFloat(12.6f, 0);
    CMS_TEST_CHECK(negative == "13");
    negative.clear();
    negative.appendFloat(0.125f, 9);
    CMS_TEST_CHECK(negative == "0.125000000");
    assertInvariant(negative);

    cms::String<16> unsupported = "keep";
    unsupported.appendFloat(std::numeric_limits<float>::quiet_NaN(), 2);
    CMS_TEST_CHECK(unsupported == "keep");
    unsupported.appendFloat(std::numeric_limits<float>::infinity(), 2);
    CMS_TEST_CHECK(unsupported == "keep");
    unsupported.appendFloat(-std::numeric_limits<float>::infinity(), 2);
    CMS_TEST_CHECK(unsupported == "keep");
    unsupported.appendFloat(std::numeric_limits<float>::max(), 2);
    CMS_TEST_CHECK(unsupported == "keep");
    assertInvariant(unsupported);

    char raw[32] = "raw";
    size_t rawLen = 3;
    const unsigned long halfRange = std::numeric_limits<unsigned long>::max() / 2UL + 1UL;
    const double upperExclusive = static_cast<double>(halfRange) * 2.0;
    cms::string::appendFloat(raw, sizeof(raw), rawLen, upperExclusive, 0);
    CMS_TEST_CHECK(rawLen == 3);
    CMS_TEST_CHECK(std::strcmp(raw, "raw") == 0);
}

static void assertUnsigned(unsigned long value, const char* expected) {
    cms::String<64> out;
    out << value;
    CMS_TEST_CHECK(out == expected);
    assertInvariant(out);
}

static void testUnsignedLong() {
    assertUnsigned(0UL, "0");
    assertUnsigned(2147483647UL, "2147483647");
    assertUnsigned(2147483648UL, "2147483648");
    assertUnsigned(4000000000UL, "4000000000");
    assertUnsigned(4294967295UL, "4294967295");
#if ULONG_MAX > 0xFFFFFFFFUL
    assertUnsigned(ULONG_MAX, "18446744073709551615");
#else
    assertUnsigned(ULONG_MAX, "4294967295");
#endif
}

static void testSanitizeUtf8() {
    char expanded[16] = { static_cast<char>(0xFF), 'A', 'B', '\0' };
    size_t len = cms::string::sanitizeUtf8(expanded, sizeof(expanded));
    const char expected[] = "\xEF\xBF\xBD" "AB";
    CMS_TEST_CHECK(len == sizeof(expected) - 1);
    CMS_TEST_CHECK(std::memcmp(expanded, expected, sizeof(expected)) == 0);
    CMS_TEST_CHECK(cms::string::validateUtf8(expanded));

    char limited[4] = { 'A', static_cast<char>(0xFF), 'B', '\0' };
    len = cms::string::sanitizeUtf8(limited, sizeof(limited));
    CMS_TEST_CHECK(len == 1);
    CMS_TEST_CHECK(std::strcmp(limited, "A") == 0);
    CMS_TEST_CHECK(cms::string::validateUtf8(limited));

    char valid[] = "\xC2\xA2" "\xED\x95\x9C" "\xF0\x9F\x98\x80";
    const char validExpected[] = "\xC2\xA2" "\xED\x95\x9C" "\xF0\x9F\x98\x80";
    len = cms::string::sanitizeUtf8(valid, sizeof(valid));
    CMS_TEST_CHECK(len == sizeof(validExpected) - 1);
    CMS_TEST_CHECK(std::memcmp(valid, validExpected, sizeof(validExpected)) == 0);

    char consecutive[16] = { static_cast<char>(0xFF), static_cast<char>(0xFE), '\0' };
    len = cms::string::sanitizeUtf8(consecutive, sizeof(consecutive));
    CMS_TEST_CHECK(len == 6 && cms::string::validateUtf8(consecutive));

    char truncated[16] = { static_cast<char>(0xE2), static_cast<char>(0x82), '\0' };
    len = cms::string::sanitizeUtf8(truncated, sizeof(truncated));
    CMS_TEST_CHECK(len == 6 && cms::string::validateUtf8(truncated));

    char overlong[16] = { static_cast<char>(0xC0), static_cast<char>(0xAF), '\0' };
    len = cms::string::sanitizeUtf8(overlong, sizeof(overlong));
    CMS_TEST_CHECK(len == 6 && cms::string::validateUtf8(overlong));

    char surrogate[16] = { static_cast<char>(0xED), static_cast<char>(0xA0), static_cast<char>(0x80), '\0' };
    len = cms::string::sanitizeUtf8(surrogate, sizeof(surrogate));
    CMS_TEST_CHECK(len == 9 && cms::string::validateUtf8(surrogate));
}

static void testSubstringBoundary() {
    const char source[] = "\xED\x95\x9C" "A";
    char tooSmall[3];
    char exact[4];
    CMS_TEST_REQUIRE(cms::string::substring(source, tooSmall, sizeof(tooSmall), 0) == 0);
    CMS_TEST_REQUIRE(tooSmall[0] == '\0');
    CMS_TEST_CHECK(std::strcmp(tooSmall, "") == 0);
    CMS_TEST_CHECK(cms::string::validateUtf8(tooSmall));
    CMS_TEST_REQUIRE(cms::string::substring(source, exact, sizeof(exact), 0) == 3);
    CMS_TEST_REQUIRE(exact[3] == '\0');
    CMS_TEST_CHECK(std::memcmp(exact, "\xED\x95\x9C", 4) == 0);
    CMS_TEST_CHECK(cms::string::validateUtf8(exact));

    const char emojiSource[] = "\xF0\x9F\x98\x80" "A";
    char emojiTooSmall[4];
    char emojiExact[5];
    CMS_TEST_REQUIRE(cms::string::substring(emojiSource, emojiTooSmall, sizeof(emojiTooSmall), 0, 1) == 0);
    CMS_TEST_REQUIRE(emojiTooSmall[0] == '\0');
    CMS_TEST_CHECK(cms::string::validateUtf8(emojiTooSmall));
    CMS_TEST_REQUIRE(cms::string::substring(emojiSource, emojiExact, sizeof(emojiExact), 0, 1) == 4);
    CMS_TEST_REQUIRE(emojiExact[4] == '\0');
    CMS_TEST_CHECK(std::memcmp(emojiExact, "\xF0\x9F\x98\x80", 5) == 0);
}

static void testSplitRemainder() {
    char mutableText[] = "a:b:c:d";
    char* parts[2];
    cms::string::Token tokens[2];
    CMS_TEST_REQUIRE(cms::string::split(mutableText, ':', parts, 2) == 2);
    CMS_TEST_REQUIRE(parts[0] != nullptr);
    CMS_TEST_REQUIRE(parts[1] != nullptr);
    CMS_TEST_CHECK(std::strcmp(parts[0], "a") == 0);
    CMS_TEST_CHECK(std::strcmp(parts[1], "b:c:d") == 0);

    CMS_TEST_REQUIRE(cms::string::split("a:b:c:d", ':', tokens, 2) == 2);
    CMS_TEST_REQUIRE(tokens[0].ptr != nullptr);
    CMS_TEST_REQUIRE(tokens[1].ptr != nullptr);
    CMS_TEST_CHECK(tokens[0].len == 1 && std::memcmp(tokens[0].ptr, "a", 1) == 0);
    CMS_TEST_CHECK(tokens[1].len == 5 && std::memcmp(tokens[1].ptr, "b:c:d", 5) == 0);

    char oneMutable[] = "a:b:c";
    char* onePart[1];
    cms::string::Token oneToken[1];
    CMS_TEST_REQUIRE(cms::string::split(oneMutable, ':', onePart, 1) == 1);
    CMS_TEST_REQUIRE(onePart[0] != nullptr);
    CMS_TEST_CHECK(std::strcmp(onePart[0], "a:b:c") == 0);
    CMS_TEST_REQUIRE(cms::string::split("a:b:c", ':', oneToken, 1) == 1);
    CMS_TEST_REQUIRE(oneToken[0].ptr != nullptr);
    CMS_TEST_CHECK(oneToken[0].len == 5 && std::memcmp(oneToken[0].ptr, "a:b:c", 5) == 0);

    char edgeMutable[] = ":a::";
    char* edgeParts[4];
    cms::string::Token edgeTokens[4];
    CMS_TEST_REQUIRE(cms::string::split(edgeMutable, ':', edgeParts, 4) == 4);
    CMS_TEST_REQUIRE(edgeParts[0] != nullptr);
    CMS_TEST_REQUIRE(edgeParts[1] != nullptr);
    CMS_TEST_REQUIRE(edgeParts[2] != nullptr);
    CMS_TEST_REQUIRE(edgeParts[3] != nullptr);
    CMS_TEST_CHECK(std::strcmp(edgeParts[0], "") == 0);
    CMS_TEST_CHECK(std::strcmp(edgeParts[1], "a") == 0);
    CMS_TEST_CHECK(std::strcmp(edgeParts[2], "") == 0);
    CMS_TEST_CHECK(std::strcmp(edgeParts[3], "") == 0);
    CMS_TEST_REQUIRE(cms::string::split(":a::", ':', edgeTokens, 4) == 4);
    CMS_TEST_REQUIRE(edgeTokens[1].ptr != nullptr);
    CMS_TEST_CHECK(edgeTokens[0].len == 0);
    CMS_TEST_CHECK(edgeTokens[1].len == 1 && edgeTokens[1].ptr[0] == 'a');
    CMS_TEST_CHECK(edgeTokens[2].len == 0);
    CMS_TEST_CHECK(edgeTokens[3].len == 0);
}

int main() {
    static_assert(!std::is_copy_constructible<cms::StringBase>::value, "StringBase must not be copy constructible");
    static_assert(!std::is_move_constructible<cms::StringBase>::value, "StringBase must not be move constructible");
    static_assert(std::is_copy_assignable<cms::StringBase>::value, "StringBase content copy assignment must remain available");
    static_assert(!std::is_move_assignable<cms::StringBase>::value, "StringBase must not be move assignable");
    static_assert(!std::is_copy_constructible<cms::ThreadSafeQueue<int, 4>>::value, "ThreadSafeQueue must not be copyable");
    static_assert(!std::is_copy_assignable<cms::ThreadSafeQueue<int, 4>>::value, "ThreadSafeQueue must not be copy assignable");
    static_assert(!std::is_move_constructible<cms::ThreadSafeQueue<int, 4>>::value, "ThreadSafeQueue must not be movable");
    static_assert(!std::is_move_assignable<cms::ThreadSafeQueue<int, 4>>::value, "ThreadSafeQueue must not be move assignable");

    testCopyAndMove();
    testPointerConstruction();
    testTransactionalNumbers();
    testUnsignedLong();
    testSanitizeUtf8();
    testSubstringBoundary();
    testSplitRemainder();
    return cms::test::finish();
}
