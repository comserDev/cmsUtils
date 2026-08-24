#include <cassert>
#include <climits>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

#ifdef NDEBUG
#error Regression tests require assert to remain enabled
#endif

#include "../src/cmsString.h"
#include "../src/cmsQueue.h"

template <size_t N>
static void assertInvariant(const cms::String<N>& value) {
    assert(value.c_str() != nullptr);
    assert(value.length() < value.capacity());
    assert(value.c_str()[value.length()] == '\0');
    assert(std::strlen(value.c_str()) == value.length());
}

static cms::String<32> returnString() {
    cms::String<32> value = "return";
    return value;
}

static void testCopyAndMove() {
    cms::String<32> a = "abc";
    cms::String<32> b(a);
    assert(a.c_str() != b.c_str());
    b += "X";
    assert(a == "abc");
    assert(b == "abcX");

    cms::String<32> c(std::move(b));
    assert(c.c_str() != b.c_str());
    assert(c == "abcX");
    assert(b == "");

    cms::String<32> d;
    d = a;
    assert(d.c_str() != a.c_str());
    d = std::move(c);
    assert(d.c_str() != c.c_str());
    assert(d == "abcX");
    assert(c == "");

    cms::String<32> sum = a + "Z";
    assert(sum.c_str() != a.c_str());
    assert(a == "abc");
    assert(sum == "abcZ");

    cms::String<32> returned = returnString();
    assert(returned == "return");
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
    assert(value == "pointer");
    assert(nullValue == "");
    assertInvariant(value);
    assertInvariant(nullValue);
}

static void testTransactionalNumbers() {
    cms::String<4> integer = "A";
    integer.appendInt(-12);
    assert(integer == "A");
    assertInvariant(integer);

    cms::String<4> positive = "A";
    positive.appendInt(123);
    assert(positive == "A");
    assertInvariant(positive);

    cms::String<4> unsignedFailure = "A";
    unsignedFailure.appendUInt(123UL);
    assert(unsignedFailure == "A");
    assertInvariant(unsignedFailure);

    cms::String<5> floating = "X";
    floating.appendFloat(1.25f, 2);
    assert(floating == "X");
    assertInvariant(floating);

    cms::String<32> enough = "N=";
    enough.appendInt(-123);
    assert(enough == "N=-123");
    enough.appendFloat(1.25f, 2);
    assert(enough == "N=-1231.25");
    assertInvariant(enough);

    cms::String<64> limits;
    limits.appendInt(LONG_MIN);
#if LONG_MAX == 2147483647L
    assert(limits == "-2147483648");
#else
    assert(limits == "-9223372036854775808");
#endif
    limits.clear();
    limits.appendInt(LONG_MAX);
#if LONG_MAX == 2147483647L
    assert(limits == "2147483647");
#else
    assert(limits == "9223372036854775807");
#endif
    assertInvariant(limits);

    cms::String<64> negative;
    negative.appendFloat(-12.5f, 2);
    assert(negative == "-12.50");
    negative.clear();
    negative.appendFloat(12.6f, 0);
    assert(negative == "13");
    negative.clear();
    negative.appendFloat(0.125f, 9);
    assert(negative == "0.125000000");
    assertInvariant(negative);

    cms::String<16> unsupported = "keep";
    unsupported.appendFloat(std::numeric_limits<float>::quiet_NaN(), 2);
    assert(unsupported == "keep");
    unsupported.appendFloat(std::numeric_limits<float>::infinity(), 2);
    assert(unsupported == "keep");
    unsupported.appendFloat(-std::numeric_limits<float>::infinity(), 2);
    assert(unsupported == "keep");
    unsupported.appendFloat(std::numeric_limits<float>::max(), 2);
    assert(unsupported == "keep");
    assertInvariant(unsupported);

    char raw[32] = "raw";
    size_t rawLen = 3;
    const unsigned long halfRange = std::numeric_limits<unsigned long>::max() / 2UL + 1UL;
    const double upperExclusive = static_cast<double>(halfRange) * 2.0;
    cms::string::appendFloat(raw, sizeof(raw), rawLen, upperExclusive, 0);
    assert(rawLen == 3);
    assert(std::strcmp(raw, "raw") == 0);
}

static void assertUnsigned(unsigned long value, const char* expected) {
    cms::String<64> out;
    out << value;
    assert(out == expected);
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
    assert(len == sizeof(expected) - 1);
    assert(std::memcmp(expanded, expected, sizeof(expected)) == 0);
    assert(cms::string::validateUtf8(expanded));

    char limited[4] = { 'A', static_cast<char>(0xFF), 'B', '\0' };
    len = cms::string::sanitizeUtf8(limited, sizeof(limited));
    assert(len == 1);
    assert(std::strcmp(limited, "A") == 0);
    assert(cms::string::validateUtf8(limited));

    char valid[] = "\xC2\xA2" "\xED\x95\x9C" "\xF0\x9F\x98\x80";
    const char validExpected[] = "\xC2\xA2" "\xED\x95\x9C" "\xF0\x9F\x98\x80";
    len = cms::string::sanitizeUtf8(valid, sizeof(valid));
    assert(len == sizeof(validExpected) - 1);
    assert(std::memcmp(valid, validExpected, sizeof(validExpected)) == 0);

    char consecutive[16] = { static_cast<char>(0xFF), static_cast<char>(0xFE), '\0' };
    len = cms::string::sanitizeUtf8(consecutive, sizeof(consecutive));
    assert(len == 6 && cms::string::validateUtf8(consecutive));

    char truncated[16] = { static_cast<char>(0xE2), static_cast<char>(0x82), '\0' };
    len = cms::string::sanitizeUtf8(truncated, sizeof(truncated));
    assert(len == 6 && cms::string::validateUtf8(truncated));

    char overlong[16] = { static_cast<char>(0xC0), static_cast<char>(0xAF), '\0' };
    len = cms::string::sanitizeUtf8(overlong, sizeof(overlong));
    assert(len == 6 && cms::string::validateUtf8(overlong));

    char surrogate[16] = { static_cast<char>(0xED), static_cast<char>(0xA0), static_cast<char>(0x80), '\0' };
    len = cms::string::sanitizeUtf8(surrogate, sizeof(surrogate));
    assert(len == 9 && cms::string::validateUtf8(surrogate));
}

static void testSubstringBoundary() {
    const char source[] = "\xED\x95\x9C" "A";
    char tooSmall[3];
    char exact[4];
    assert(cms::string::substring(source, tooSmall, sizeof(tooSmall), 0) == 0);
    assert(std::strcmp(tooSmall, "") == 0);
    assert(cms::string::validateUtf8(tooSmall));
    assert(cms::string::substring(source, exact, sizeof(exact), 0) == 3);
    assert(std::memcmp(exact, "\xED\x95\x9C", 4) == 0);
    assert(cms::string::validateUtf8(exact));

    const char emojiSource[] = "\xF0\x9F\x98\x80" "A";
    char emojiTooSmall[4];
    char emojiExact[5];
    assert(cms::string::substring(emojiSource, emojiTooSmall, sizeof(emojiTooSmall), 0, 1) == 0);
    assert(cms::string::validateUtf8(emojiTooSmall));
    assert(cms::string::substring(emojiSource, emojiExact, sizeof(emojiExact), 0, 1) == 4);
    assert(std::memcmp(emojiExact, "\xF0\x9F\x98\x80", 5) == 0);
}

static void testSplitRemainder() {
    char mutableText[] = "a:b:c:d";
    char* parts[2];
    cms::string::Token tokens[2];
    assert(cms::string::split(mutableText, ':', parts, 2) == 2);
    assert(std::strcmp(parts[0], "a") == 0);
    assert(std::strcmp(parts[1], "b:c:d") == 0);

    assert(cms::string::split("a:b:c:d", ':', tokens, 2) == 2);
    assert(tokens[0].len == 1 && std::memcmp(tokens[0].ptr, "a", 1) == 0);
    assert(tokens[1].len == 5 && std::memcmp(tokens[1].ptr, "b:c:d", 5) == 0);

    char oneMutable[] = "a:b:c";
    char* onePart[1];
    cms::string::Token oneToken[1];
    assert(cms::string::split(oneMutable, ':', onePart, 1) == 1);
    assert(std::strcmp(onePart[0], "a:b:c") == 0);
    assert(cms::string::split("a:b:c", ':', oneToken, 1) == 1);
    assert(oneToken[0].len == 5 && std::memcmp(oneToken[0].ptr, "a:b:c", 5) == 0);

    char edgeMutable[] = ":a::";
    char* edgeParts[4];
    cms::string::Token edgeTokens[4];
    assert(cms::string::split(edgeMutable, ':', edgeParts, 4) == 4);
    assert(std::strcmp(edgeParts[0], "") == 0);
    assert(std::strcmp(edgeParts[1], "a") == 0);
    assert(std::strcmp(edgeParts[2], "") == 0);
    assert(std::strcmp(edgeParts[3], "") == 0);
    assert(cms::string::split(":a::", ':', edgeTokens, 4) == 4);
    assert(edgeTokens[0].len == 0);
    assert(edgeTokens[1].len == 1 && edgeTokens[1].ptr[0] == 'a');
    assert(edgeTokens[2].len == 0);
    assert(edgeTokens[3].len == 0);
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
    return 0;
}
