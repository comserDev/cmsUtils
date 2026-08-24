#include <cstdio>

#include <cms/utf8.h>

#include "test.h"

namespace {

constexpr char byte(unsigned int value) noexcept {
    return static_cast<char>(value);
}

void checkBytes(
    cms::StringView actual,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(actual.size() == expectedSize);
    if (expectedSize > 0) {
        CMS_TEST_REQUIRE(actual.data() != nullptr);
        CMS_TEST_REQUIRE(expected != nullptr);
    }

    for (std::size_t index = 0; index < expectedSize; ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

void checkValidDecode(
    cms::StringView input,
    char32_t codePoint,
    std::size_t bytes) {
    const cms::utf8::DecodeResult result = cms::utf8::decodeNext(input, 0);
    CMS_TEST_REQUIRE(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.codePoint == codePoint);
    CMS_TEST_CHECK(result.bytes == bytes);
}

void checkInvalidDecode(cms::StringView input) {
    const cms::utf8::DecodeResult result = cms::utf8::decodeNext(input, 0);
    CMS_TEST_CHECK(result.status == cms::Status::invalid_utf8);
    CMS_TEST_CHECK(result.codePoint == static_cast<char32_t>(0xFFFD));
    CMS_TEST_CHECK(result.bytes == 1);
    CMS_TEST_CHECK(cms::utf8::validate(input) == cms::Status::invalid_utf8);
}

void checkCount(
    cms::StringView input,
    std::size_t expectedCount,
    std::size_t expectedConsumed) {
    const cms::ParseResult<std::size_t> result = cms::utf8::count(input);
    CMS_TEST_CHECK(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.value == expectedCount);
    CMS_TEST_CHECK(result.consumed == expectedConsumed);
}

void checkSubstring(
    cms::StringView input,
    std::size_t first,
    std::size_t count,
    const char* expected,
    std::size_t expectedSize) {
    char storage[64] = "old";
    std::size_t size = 3;
    cms::StringBuffer output(storage, sizeof(storage), size);

    const cms::WriteResult result =
        cms::utf8::substring(input, first, count, output);
    CMS_TEST_REQUIRE(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.written == expectedSize);
    CMS_TEST_CHECK(result.required == expectedSize);
    CMS_TEST_REQUIRE(output.valid());
    checkBytes(output.view(), expected, expectedSize);
}

void checkSanitize(
    cms::StringView input,
    const char* expected,
    std::size_t expectedSize) {
    char storage[64] = "old";
    std::size_t size = 3;
    cms::StringBuffer output(storage, sizeof(storage), size);

    const cms::WriteResult result = cms::utf8::sanitize(input, output);
    CMS_TEST_REQUIRE(result.status == cms::Status::ok);
    CMS_TEST_CHECK(result.written == expectedSize);
    CMS_TEST_CHECK(result.required == expectedSize);
    CMS_TEST_REQUIRE(output.valid());
    checkBytes(output.view(), expected, expectedSize);
    CMS_TEST_CHECK(cms::utf8::validate(output.view()) == cms::Status::ok);
}

} // namespace

int main() {
    const char ascii[] = {'A'};
    const char embeddedNul[] = {'A', '\0', 'B'};
    const char cent[] = {byte(0xC2), byte(0xA2)};
    const char korean[] = {byte(0xEA), byte(0xB0), byte(0x80)};
    const char emoji[] = {
        byte(0xF0), byte(0x9F), byte(0x98), byte(0x80)};
    const char mixed[] = {
        'A',
        byte(0xC2), byte(0xA2),
        byte(0xEA), byte(0xB0), byte(0x80),
        byte(0xF0), byte(0x9F), byte(0x98), byte(0x80)};

    checkValidDecode(cms::StringView(ascii, sizeof(ascii)), U'A', 1);
    checkValidDecode(cms::StringView(cent, sizeof(cent)), 0x00A2, 2);
    checkValidDecode(cms::StringView(korean, sizeof(korean)), 0xAC00, 3);
    checkValidDecode(cms::StringView(emoji, sizeof(emoji)), 0x1F600, 4);

    const char u007f[] = {byte(0x7F)};
    const char u0080[] = {byte(0xC2), byte(0x80)};
    const char u07ff[] = {byte(0xDF), byte(0xBF)};
    const char u0800[] = {byte(0xE0), byte(0xA0), byte(0x80)};
    const char ud7ff[] = {byte(0xED), byte(0x9F), byte(0xBF)};
    const char ue000[] = {byte(0xEE), byte(0x80), byte(0x80)};
    const char uffff[] = {byte(0xEF), byte(0xBF), byte(0xBF)};
    const char u10000[] = {
        byte(0xF0), byte(0x90), byte(0x80), byte(0x80)};
    const char u10ffff[] = {
        byte(0xF4), byte(0x8F), byte(0xBF), byte(0xBF)};

    checkValidDecode(cms::StringView(u007f, sizeof(u007f)), 0x007F, 1);
    checkValidDecode(cms::StringView(u0080, sizeof(u0080)), 0x0080, 2);
    checkValidDecode(cms::StringView(u07ff, sizeof(u07ff)), 0x07FF, 2);
    checkValidDecode(cms::StringView(u0800, sizeof(u0800)), 0x0800, 3);
    checkValidDecode(cms::StringView(ud7ff, sizeof(ud7ff)), 0xD7FF, 3);
    checkValidDecode(cms::StringView(ue000, sizeof(ue000)), 0xE000, 3);
    checkValidDecode(cms::StringView(uffff, sizeof(uffff)), 0xFFFF, 3);
    checkValidDecode(cms::StringView(u10000, sizeof(u10000)), 0x10000, 4);
    checkValidDecode(cms::StringView(u10ffff, sizeof(u10ffff)), 0x10FFFF, 4);

    CMS_TEST_CHECK(cms::utf8::validate(cms::StringView()) == cms::Status::ok);
    CMS_TEST_CHECK(
        cms::utf8::validate(cms::StringView(mixed, sizeof(mixed)))
        == cms::Status::ok);
    CMS_TEST_CHECK(
        cms::utf8::validate(cms::StringView(embeddedNul, sizeof(embeddedNul)))
        == cms::Status::ok);

    const char isolatedContinuation[] = {byte(0x80)};
    const char c0[] = {byte(0xC0)};
    const char c1[] = {byte(0xC1)};
    const char overlongTwo[] = {byte(0xC0), byte(0xAF)};
    const char overlongThree[] = {byte(0xE0), byte(0x80), byte(0x80)};
    const char overlongFour[] = {
        byte(0xF0), byte(0x80), byte(0x80), byte(0x80)};
    const char truncatedTwo[] = {byte(0xC2)};
    const char truncatedThree[] = {byte(0xE2), byte(0x82)};
    const char truncatedFour[] = {byte(0xF0), byte(0x90), byte(0x80)};
    const char surrogateLower[] = {byte(0xED), byte(0xA0), byte(0x80)};
    const char surrogateInterior[] = {byte(0xED), byte(0xAF), byte(0xBF)};
    const char surrogateUpper[] = {byte(0xED), byte(0xBF), byte(0xBF)};
    const char aboveMaximum[] = {
        byte(0xF4), byte(0x90), byte(0x80), byte(0x80)};
    const char f5[] = {byte(0xF5)};
    const char ff[] = {byte(0xFF)};
    const char malformedTwo[] = {byte(0xC2), 'A'};
    const char malformedThreeSecond[] = {byte(0xE1), 'A', byte(0x80)};
    const char malformedThreeThird[] = {byte(0xE1), byte(0x80), 'A'};
    const char malformedFourSecond[] = {
        byte(0xF1), 'A', byte(0x80), byte(0x80)};
    const char malformedFourThird[] = {
        byte(0xF1), byte(0x80), 'A', byte(0x80)};
    const char malformedFourFourth[] = {
        byte(0xF1), byte(0x80), byte(0x80), 'A'};

    checkInvalidDecode(cms::StringView(
        isolatedContinuation, sizeof(isolatedContinuation)));
    checkInvalidDecode(cms::StringView(c0, sizeof(c0)));
    checkInvalidDecode(cms::StringView(c1, sizeof(c1)));
    checkInvalidDecode(cms::StringView(overlongTwo, sizeof(overlongTwo)));
    checkInvalidDecode(cms::StringView(overlongThree, sizeof(overlongThree)));
    checkInvalidDecode(cms::StringView(overlongFour, sizeof(overlongFour)));
    checkInvalidDecode(cms::StringView(truncatedTwo, sizeof(truncatedTwo)));
    checkInvalidDecode(cms::StringView(truncatedThree, sizeof(truncatedThree)));
    checkInvalidDecode(cms::StringView(truncatedFour, sizeof(truncatedFour)));
    checkInvalidDecode(cms::StringView(surrogateLower, sizeof(surrogateLower)));
    checkInvalidDecode(cms::StringView(
        surrogateInterior, sizeof(surrogateInterior)));
    checkInvalidDecode(cms::StringView(surrogateUpper, sizeof(surrogateUpper)));
    checkInvalidDecode(cms::StringView(aboveMaximum, sizeof(aboveMaximum)));
    checkInvalidDecode(cms::StringView(f5, sizeof(f5)));
    checkInvalidDecode(cms::StringView(ff, sizeof(ff)));
    checkInvalidDecode(cms::StringView(malformedTwo, sizeof(malformedTwo)));
    checkInvalidDecode(cms::StringView(
        malformedThreeSecond, sizeof(malformedThreeSecond)));
    checkInvalidDecode(cms::StringView(
        malformedThreeThird, sizeof(malformedThreeThird)));
    checkInvalidDecode(cms::StringView(
        malformedFourSecond, sizeof(malformedFourSecond)));
    checkInvalidDecode(cms::StringView(
        malformedFourThird, sizeof(malformedFourThird)));
    checkInvalidDecode(cms::StringView(
        malformedFourFourth, sizeof(malformedFourFourth)));

    const cms::utf8::DecodeResult atEnd =
        cms::utf8::decodeNext(cms::StringView(ascii, sizeof(ascii)), 1);
    CMS_TEST_CHECK(atEnd.status == cms::Status::out_of_range);
    CMS_TEST_CHECK(atEnd.codePoint == 0);
    CMS_TEST_CHECK(atEnd.bytes == 0);
    const cms::utf8::DecodeResult pastEnd =
        cms::utf8::decodeNext(cms::StringView(ascii, sizeof(ascii)), 2);
    CMS_TEST_CHECK(pastEnd.status == cms::Status::out_of_range);
    CMS_TEST_CHECK(pastEnd.codePoint == 0);
    CMS_TEST_CHECK(pastEnd.bytes == 0);

    checkCount(cms::StringView(), 0, 0);
    const char asciiText[] = {'A', 'B', 'C'};
    checkCount(cms::StringView(asciiText, sizeof(asciiText)), 3, 3);
    const char koreanText[] = {
        byte(0xEA), byte(0xB0), byte(0x80),
        byte(0xEB), byte(0x82), byte(0x98)};
    checkCount(cms::StringView(koreanText, sizeof(koreanText)), 2, 6);
    checkCount(cms::StringView(emoji, sizeof(emoji)), 1, 4);
    checkCount(cms::StringView(mixed, sizeof(mixed)), 4, sizeof(mixed));
    checkCount(
        cms::StringView(embeddedNul, sizeof(embeddedNul)),
        3,
        sizeof(embeddedNul));

    const char invalidAfterAscii[] = {'A', byte(0xE2), byte(0x82)};
    const cms::ParseResult<std::size_t> invalidCount =
        cms::utf8::count(cms::StringView(
            invalidAfterAscii,
            sizeof(invalidAfterAscii)));
    CMS_TEST_CHECK(invalidCount.status == cms::Status::invalid_utf8);
    CMS_TEST_CHECK(invalidCount.value == 1);
    CMS_TEST_CHECK(invalidCount.consumed == 1);

    const char expectedBc[] = {'B', 'C'};
    checkSubstring(
        cms::StringView(asciiText, sizeof(asciiText)),
        1,
        100,
        expectedBc,
        sizeof(expectedBc));
    const char secondKorean[] = {byte(0xEB), byte(0x82), byte(0x98)};
    checkSubstring(
        cms::StringView(koreanText, sizeof(koreanText)),
        1,
        1,
        secondKorean,
        sizeof(secondKorean));
    checkSubstring(
        cms::StringView(emoji, sizeof(emoji)),
        0,
        1,
        emoji,
        sizeof(emoji));
    const char mixedMiddle[] = {
        byte(0xC2), byte(0xA2), byte(0xEA), byte(0xB0), byte(0x80)};
    checkSubstring(
        cms::StringView(mixed, sizeof(mixed)),
        1,
        2,
        mixedMiddle,
        sizeof(mixedMiddle));
    checkSubstring(
        cms::StringView(asciiText, sizeof(asciiText)),
        3,
        5,
        nullptr,
        0);
    checkSubstring(
        cms::StringView(asciiText, sizeof(asciiText)),
        1,
        0,
        nullptr,
        0);

    char outOfRangeStorage[8] = "old";
    std::size_t outOfRangeSize = 3;
    cms::StringBuffer outOfRangeOutput(
        outOfRangeStorage,
        sizeof(outOfRangeStorage),
        outOfRangeSize);
    const cms::WriteResult outOfRangeResult = cms::utf8::substring(
        cms::StringView(asciiText, sizeof(asciiText)),
        4,
        1,
        outOfRangeOutput);
    CMS_TEST_CHECK(outOfRangeResult.status == cms::Status::out_of_range);
    CMS_TEST_CHECK(outOfRangeResult.written == 0);
    CMS_TEST_CHECK(outOfRangeResult.required == 0);
    CMS_TEST_CHECK(outOfRangeSize == 3);
    CMS_TEST_CHECK(outOfRangeStorage[0] == 'o');

    char exactSubstringStorage[5] = "old";
    std::size_t exactSubstringSize = 3;
    cms::StringBuffer exactSubstringOutput(
        exactSubstringStorage,
        sizeof(exactSubstringStorage),
        exactSubstringSize);
    const cms::WriteResult exactSubstring = cms::utf8::substring(
        cms::StringView(emoji, sizeof(emoji)),
        0,
        1,
        exactSubstringOutput);
    CMS_TEST_CHECK(exactSubstring.status == cms::Status::ok);
    CMS_TEST_CHECK(exactSubstring.written == 4);
    CMS_TEST_CHECK(exactSubstring.required == 4);
    CMS_TEST_CHECK(exactSubstringSize == 4);
    CMS_TEST_CHECK(exactSubstringStorage[4] == '\0');

    char shortSubstringStorage[4] = "old";
    std::size_t shortSubstringSize = 3;
    cms::StringBuffer shortSubstringOutput(
        shortSubstringStorage,
        sizeof(shortSubstringStorage),
        shortSubstringSize);
    const cms::WriteResult shortSubstring = cms::utf8::substring(
        cms::StringView(emoji, sizeof(emoji)),
        0,
        1,
        shortSubstringOutput);
    CMS_TEST_CHECK(shortSubstring.status == cms::Status::no_space);
    CMS_TEST_CHECK(shortSubstring.written == 0);
    CMS_TEST_CHECK(shortSubstring.required == 4);
    CMS_TEST_CHECK(shortSubstringSize == 3);
    CMS_TEST_CHECK(shortSubstringStorage[0] == 'o');
    CMS_TEST_CHECK(shortSubstringStorage[3] == '\0');

    char invalidSubstringStorage[8] = "old";
    std::size_t invalidSubstringSize = 3;
    cms::StringBuffer invalidSubstringOutput(
        invalidSubstringStorage,
        sizeof(invalidSubstringStorage),
        invalidSubstringSize);
    const cms::WriteResult invalidSubstring = cms::utf8::substring(
        cms::StringView(invalidAfterAscii, sizeof(invalidAfterAscii)),
        0,
        1,
        invalidSubstringOutput);
    CMS_TEST_CHECK(invalidSubstring.status == cms::Status::invalid_utf8);
    CMS_TEST_CHECK(invalidSubstring.written == 0);
    CMS_TEST_CHECK(invalidSubstring.required == 0);
    CMS_TEST_CHECK(invalidSubstringSize == 3);
    CMS_TEST_CHECK(invalidSubstringStorage[0] == 'o');

    const char invalidBeforeSelection[] = {byte(0x80), 'A'};
    const cms::WriteResult invalidBeforeSubstring = cms::utf8::substring(
        cms::StringView(
            invalidBeforeSelection,
            sizeof(invalidBeforeSelection)),
        1,
        1,
        invalidSubstringOutput);
    CMS_TEST_CHECK(
        invalidBeforeSubstring.status == cms::Status::invalid_utf8);
    CMS_TEST_CHECK(invalidBeforeSubstring.written == 0);
    CMS_TEST_CHECK(invalidBeforeSubstring.required == 0);
    CMS_TEST_CHECK(invalidSubstringSize == 3);
    CMS_TEST_CHECK(invalidSubstringStorage[0] == 'o');

    cms::StringBuffer defaultOutput;
    const cms::WriteResult defaultSubstring = cms::utf8::substring(
        cms::StringView(asciiText, sizeof(asciiText)),
        0,
        1,
        defaultOutput);
    CMS_TEST_CHECK(defaultSubstring.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(defaultSubstring.written == 0);
    CMS_TEST_CHECK(defaultSubstring.required == 0);

    char damagedStorage[8] = "old";
    std::size_t damagedSize = 3;
    cms::StringBuffer damagedOutput(
        damagedStorage,
        sizeof(damagedStorage),
        damagedSize);
    damagedSize = sizeof(damagedStorage);
    const cms::WriteResult damagedSubstring = cms::utf8::substring(
        cms::StringView(asciiText, sizeof(asciiText)),
        0,
        1,
        damagedOutput);
    CMS_TEST_CHECK(damagedSubstring.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(damagedSubstring.written == 0);
    CMS_TEST_CHECK(damagedSubstring.required == 0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');

    checkSanitize(cms::StringView(mixed, sizeof(mixed)), mixed, sizeof(mixed));
    const char replacementOne[] = {byte(0xEF), byte(0xBF), byte(0xBD)};
    const char replacementTwo[] = {
        byte(0xEF), byte(0xBF), byte(0xBD),
        byte(0xEF), byte(0xBF), byte(0xBD)};
    const char replacementThree[] = {
        byte(0xEF), byte(0xBF), byte(0xBD),
        byte(0xEF), byte(0xBF), byte(0xBD),
        byte(0xEF), byte(0xBF), byte(0xBD)};
    checkSanitize(
        cms::StringView(isolatedContinuation, sizeof(isolatedContinuation)),
        replacementOne,
        sizeof(replacementOne));
    const char consecutiveInvalid[] = {byte(0x80), byte(0xFF)};
    checkSanitize(
        cms::StringView(consecutiveInvalid, sizeof(consecutiveInvalid)),
        replacementTwo,
        sizeof(replacementTwo));
    checkSanitize(
        cms::StringView(overlongTwo, sizeof(overlongTwo)),
        replacementTwo,
        sizeof(replacementTwo));
    checkSanitize(
        cms::StringView(truncatedThree, sizeof(truncatedThree)),
        replacementTwo,
        sizeof(replacementTwo));
    checkSanitize(
        cms::StringView(surrogateLower, sizeof(surrogateLower)),
        replacementThree,
        sizeof(replacementThree));
    const char validInvalidValid[] = {'A', byte(0x80), 'B'};
    const char sanitizedMiddle[] = {
        'A', byte(0xEF), byte(0xBF), byte(0xBD), 'B'};
    checkSanitize(
        cms::StringView(validInvalidValid, sizeof(validInvalidValid)),
        sanitizedMiddle,
        sizeof(sanitizedMiddle));
    checkSanitize(
        cms::StringView(embeddedNul, sizeof(embeddedNul)),
        embeddedNul,
        sizeof(embeddedNul));

    char exactSanitizeStorage[4] = "old";
    std::size_t exactSanitizeSize = 3;
    cms::StringBuffer exactSanitizeOutput(
        exactSanitizeStorage,
        sizeof(exactSanitizeStorage),
        exactSanitizeSize);
    const cms::WriteResult exactSanitize = cms::utf8::sanitize(
        cms::StringView(isolatedContinuation, sizeof(isolatedContinuation)),
        exactSanitizeOutput);
    CMS_TEST_CHECK(exactSanitize.status == cms::Status::ok);
    CMS_TEST_CHECK(exactSanitize.written == 3);
    CMS_TEST_CHECK(exactSanitize.required == 3);
    CMS_TEST_CHECK(exactSanitizeSize == 3);
    CMS_TEST_CHECK(exactSanitizeStorage[3] == '\0');
    CMS_TEST_CHECK(
        cms::utf8::validate(exactSanitizeOutput.view()) == cms::Status::ok);

    char shortSanitizeStorage[3] = "x";
    std::size_t shortSanitizeSize = 1;
    cms::StringBuffer shortSanitizeOutput(
        shortSanitizeStorage,
        sizeof(shortSanitizeStorage),
        shortSanitizeSize);
    const cms::WriteResult shortSanitize = cms::utf8::sanitize(
        cms::StringView(isolatedContinuation, sizeof(isolatedContinuation)),
        shortSanitizeOutput);
    CMS_TEST_CHECK(shortSanitize.status == cms::Status::no_space);
    CMS_TEST_CHECK(shortSanitize.written == 0);
    CMS_TEST_CHECK(shortSanitize.required == 3);
    CMS_TEST_CHECK(shortSanitizeSize == 1);
    CMS_TEST_CHECK(shortSanitizeStorage[0] == 'x');
    CMS_TEST_CHECK(shortSanitizeStorage[1] == '\0');

    const cms::WriteResult defaultSanitize = cms::utf8::sanitize(
        cms::StringView(asciiText, sizeof(asciiText)),
        defaultOutput);
    CMS_TEST_CHECK(defaultSanitize.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(defaultSanitize.written == 0);
    CMS_TEST_CHECK(defaultSanitize.required == 0);

    const cms::WriteResult damagedSanitize = cms::utf8::sanitize(
        cms::StringView(asciiText, sizeof(asciiText)),
        damagedOutput);
    CMS_TEST_CHECK(damagedSanitize.status == cms::Status::invalid_argument);
    CMS_TEST_CHECK(damagedSanitize.written == 0);
    CMS_TEST_CHECK(damagedSanitize.required == 0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');

    std::printf(
        "sizeof(cms::utf8::DecodeResult)=%zu\n",
        sizeof(cms::utf8::DecodeResult));
    return cms::test::finish();
}
