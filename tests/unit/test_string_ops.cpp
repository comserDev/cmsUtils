#include <cstdio>

#include <cms/string_ops.h>

#include "test.h"

namespace {

constexpr char byte(unsigned int value) noexcept {
    return static_cast<char>(value);
}

void checkResult(
    const cms::WriteResult& result,
    cms::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
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

void checkBuffer(
    cms::StringBuffer output,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(output.valid());
    CMS_TEST_CHECK(output.size() == expectedSize);
    CMS_TEST_REQUIRE(output.data() != nullptr);
    CMS_TEST_CHECK(output.data()[output.size()] == '\0');
    checkBytes(output.view(), expected, expectedSize);
}

void checkReplace(
    cms::StringView input,
    cms::StringView needle,
    cms::StringView replacement,
    const char* expected,
    std::size_t expectedSize) {
    char storage[64] = "old";
    std::size_t size = 3;
    cms::StringBuffer output(storage, sizeof(storage), size);
    const cms::WriteResult result =
        cms::string::replaceAll(input, needle, replacement, output);
    checkResult(result, cms::Status::ok, expectedSize, expectedSize);
    checkBuffer(output, expected, expectedSize);
}

} // namespace

int main() {
    const cms::StringView empty;
    CMS_TEST_CHECK(cms::string::compare(empty, empty) == 0);
    CMS_TEST_CHECK(cms::string::compare(empty, cms::StringView("a")) == -1);
    CMS_TEST_CHECK(cms::string::compare(cms::StringView("a"), empty) == 1);
    CMS_TEST_CHECK(
        cms::string::compare(cms::StringView("abc"), cms::StringView("abc"))
        == 0);
    CMS_TEST_CHECK(
        cms::string::compare(cms::StringView("ab"), cms::StringView("abc"))
        == -1);
    CMS_TEST_CHECK(
        cms::string::compare(cms::StringView("abc"), cms::StringView("ab"))
        == 1);
    CMS_TEST_CHECK(
        cms::string::compare(cms::StringView("abc"), cms::StringView("abd"))
        == -1);
    CMS_TEST_CHECK(
        cms::string::compare(cms::StringView("abd"), cms::StringView("abc"))
        == 1);

    const char embeddedLeft[] = {'A', '\0', 'B'};
    const char embeddedRight[] = {'A', '\0', 'C'};
    CMS_TEST_CHECK(
        cms::string::compare(
            cms::StringView(embeddedLeft, sizeof(embeddedLeft)),
            cms::StringView(embeddedRight, sizeof(embeddedRight))) == -1);
    const char lowerUnsigned[] = {byte(0x7F)};
    const char higherUnsigned[] = {byte(0x80)};
    CMS_TEST_CHECK(
        cms::string::compare(
            cms::StringView(lowerUnsigned, sizeof(lowerUnsigned)),
            cms::StringView(higherUnsigned, sizeof(higherUnsigned))) == -1);
    CMS_TEST_CHECK(
        cms::string::compare(
            cms::StringView(higherUnsigned, sizeof(higherUnsigned)),
            cms::StringView(lowerUnsigned, sizeof(lowerUnsigned))) == 1);

    CMS_TEST_CHECK(cms::string::equals(empty, empty));
    CMS_TEST_CHECK(
        cms::string::equals(cms::StringView("abc"), cms::StringView("abc")));
    CMS_TEST_CHECK(
        !cms::string::equals(cms::StringView("ab"), cms::StringView("abc")));
    CMS_TEST_CHECK(
        !cms::string::equals(cms::StringView("abc"), cms::StringView("abd")));
    CMS_TEST_CHECK(cms::string::equals(
        cms::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::StringView(embeddedLeft, sizeof(embeddedLeft))));

    CMS_TEST_CHECK(cms::string::startsWith(cms::StringView("abc"), empty));
    CMS_TEST_CHECK(cms::string::startsWith(
        cms::StringView("abc"), cms::StringView("abc")));
    CMS_TEST_CHECK(cms::string::startsWith(
        cms::StringView("abc"), cms::StringView("ab")));
    CMS_TEST_CHECK(!cms::string::startsWith(
        cms::StringView("ab"), cms::StringView("abc")));
    CMS_TEST_CHECK(!cms::string::startsWith(
        cms::StringView("abc"), cms::StringView("bc")));
    const char embeddedPrefix[] = {'A', '\0'};
    CMS_TEST_CHECK(cms::string::startsWith(
        cms::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::StringView(embeddedPrefix, sizeof(embeddedPrefix))));

    CMS_TEST_CHECK(cms::string::endsWith(cms::StringView("abc"), empty));
    CMS_TEST_CHECK(cms::string::endsWith(
        cms::StringView("abc"), cms::StringView("abc")));
    CMS_TEST_CHECK(cms::string::endsWith(
        cms::StringView("abc"), cms::StringView("bc")));
    CMS_TEST_CHECK(!cms::string::endsWith(
        cms::StringView("ab"), cms::StringView("abc")));
    CMS_TEST_CHECK(!cms::string::endsWith(
        cms::StringView("abc"), cms::StringView("ab")));
    const char embeddedSuffix[] = {'\0', 'B'};
    CMS_TEST_CHECK(cms::string::endsWith(
        cms::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::StringView(embeddedSuffix, sizeof(embeddedSuffix))));

    CMS_TEST_CHECK(cms::string::find(empty, empty) == 0);
    CMS_TEST_CHECK(
        cms::string::find(empty, cms::StringView("a")) == cms::string::npos);
    CMS_TEST_CHECK(cms::string::find(cms::StringView("abc"), empty, 0) == 0);
    CMS_TEST_CHECK(cms::string::find(cms::StringView("abc"), empty, 2) == 2);
    CMS_TEST_CHECK(cms::string::find(cms::StringView("abc"), empty, 3) == 3);
    CMS_TEST_CHECK(
        cms::string::find(cms::StringView("abc"), empty, 4)
        == cms::string::npos);
    CMS_TEST_CHECK(
        cms::string::find(
            cms::StringView("abc"),
            cms::StringView("a"),
            cms::string::npos) == cms::string::npos);
    CMS_TEST_CHECK(
        cms::string::find(
            cms::StringView("abc"),
            empty,
            cms::string::npos) == cms::string::npos);
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView("abc"), cms::StringView("abc")) == 0);
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView("abcabc"), cms::StringView("ab")) == 0);
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView("abcabc"), cms::StringView("ca")) == 2);
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView("abcabc"), cms::StringView("bc"), 3) == 4);
    CMS_TEST_CHECK(
        cms::string::find(cms::StringView("abc"), cms::StringView("x"))
        == cms::string::npos);
    CMS_TEST_CHECK(
        cms::string::find(cms::StringView("abc"), cms::StringView("a"), 3)
        == cms::string::npos);
    CMS_TEST_CHECK(
        cms::string::find(cms::StringView("abc"), cms::StringView("a"), 4)
        == cms::string::npos);
    CMS_TEST_CHECK(
        cms::string::find(cms::StringView("ab"), cms::StringView("abc"))
        == cms::string::npos);
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView("aaa"), cms::StringView("aa")) == 0);

    const char embeddedHaystack[] = {'A', '\0', 'B', '\0', 'B'};
    CMS_TEST_CHECK(cms::string::find(
        cms::StringView(embeddedHaystack, sizeof(embeddedHaystack)),
        cms::StringView(embeddedSuffix, sizeof(embeddedSuffix))) == 1);

    CMS_TEST_CHECK(cms::string::findLast(empty, empty) == 0);
    CMS_TEST_CHECK(
        cms::string::findLast(empty, cms::StringView("a"))
        == cms::string::npos);
    CMS_TEST_CHECK(cms::string::findLast(
        cms::StringView("abc"), empty) == 3);
    CMS_TEST_CHECK(cms::string::findLast(
        cms::StringView("abc"), cms::StringView("b")) == 1);
    CMS_TEST_CHECK(cms::string::findLast(
        cms::StringView("ababa"), cms::StringView("ba")) == 3);
    CMS_TEST_CHECK(cms::string::findLast(
        cms::StringView("aaa"), cms::StringView("aa")) == 1);
    CMS_TEST_CHECK(
        cms::string::findLast(cms::StringView("abc"), cms::StringView("x"))
        == cms::string::npos);
    CMS_TEST_CHECK(cms::string::findLast(
        cms::StringView(embeddedHaystack, sizeof(embeddedHaystack)),
        cms::StringView(embeddedSuffix, sizeof(embeddedSuffix))) == 3);

    char copyStorage[8] = "old";
    std::size_t copySize = 3;
    cms::StringBuffer copyOutput(copyStorage, sizeof(copyStorage), copySize);
    checkResult(
        cms::string::copy(empty, copyOutput),
        cms::Status::ok,
        0,
        0);
    checkBuffer(copyOutput, nullptr, 0);
    checkResult(
        cms::string::copy(cms::StringView("abc"), copyOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(copyOutput, "abc", 3);
    checkResult(
        cms::string::copy(cms::StringView("xy"), copyOutput),
        cms::Status::ok,
        2,
        2);
    checkBuffer(copyOutput, "xy", 2);
    checkResult(
        cms::string::copy(cms::StringView("1234567"), copyOutput),
        cms::Status::ok,
        7,
        7);
    checkBuffer(copyOutput, "1234567", 7);

    const char tooLarge[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    checkResult(
        cms::string::copy(
            cms::StringView(tooLarge, sizeof(tooLarge)),
            copyOutput),
        cms::Status::no_space,
        0,
        sizeof(tooLarge));
    checkBuffer(copyOutput, "1234567", 7);

    const char embeddedInput[] = {'A', '\0', 'B'};
    checkResult(
        cms::string::copy(
            cms::StringView(embeddedInput, sizeof(embeddedInput)),
            copyOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(copyOutput, embeddedInput, sizeof(embeddedInput));

    cms::StringBuffer defaultOutput;
    checkResult(
        cms::string::copy(cms::StringView("x"), defaultOutput),
        cms::Status::invalid_argument,
        0,
        0);
    char damagedStorage[8] = "old";
    std::size_t damagedSize = 3;
    cms::StringBuffer damagedOutput(
        damagedStorage, sizeof(damagedStorage), damagedSize);
    damagedSize = sizeof(damagedStorage);
    checkResult(
        cms::string::copy(cms::StringView("x"), damagedOutput),
        cms::Status::invalid_argument,
        0,
        0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');
    CMS_TEST_CHECK(damagedStorage[1] == 'l');
    CMS_TEST_CHECK(damagedStorage[2] == 'd');
    CMS_TEST_CHECK(damagedStorage[3] == '\0');

    char appendStorage[8] = "";
    std::size_t appendSize = 0;
    cms::StringBuffer appendOutput(
        appendStorage, sizeof(appendStorage), appendSize);
    checkResult(
        cms::string::append(cms::StringView("ab"), appendOutput),
        cms::Status::ok,
        2,
        2);
    checkResult(
        cms::string::append(cms::StringView("cd"), appendOutput),
        cms::Status::ok,
        2,
        2);
    checkResult(
        cms::string::append(cms::StringView("efg"), appendOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::string::append(cms::StringView("x"), appendOutput),
        cms::Status::no_space,
        0,
        1);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::string::append(empty, appendOutput),
        cms::Status::ok,
        0,
        0);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::string::append(cms::StringView("x"), defaultOutput),
        cms::Status::invalid_argument,
        0,
        0);

    char embeddedAppendStorage[8] = "X";
    std::size_t embeddedAppendSize = 1;
    cms::StringBuffer embeddedAppend(
        embeddedAppendStorage,
        sizeof(embeddedAppendStorage),
        embeddedAppendSize);
    checkResult(
        cms::string::append(
            cms::StringView(embeddedInput, sizeof(embeddedInput)),
            embeddedAppend),
        cms::Status::ok,
        3,
        3);
    const char embeddedAppended[] = {'X', 'A', '\0', 'B'};
    checkBuffer(
        embeddedAppend,
        embeddedAppended,
        sizeof(embeddedAppended));

    char truncatedStorage[4] = "old";
    std::size_t truncatedSize = 3;
    cms::StringBuffer truncatedOutput(
        truncatedStorage, sizeof(truncatedStorage), truncatedSize);
    checkResult(
        cms::string::copyTruncated(cms::StringView("xy"), truncatedOutput),
        cms::Status::ok,
        2,
        2);
    checkBuffer(truncatedOutput, "xy", 2);
    checkResult(
        cms::string::copyTruncated(cms::StringView("abcde"), truncatedOutput),
        cms::Status::no_space,
        3,
        5);
    checkBuffer(truncatedOutput, "abc", 3);

    char zeroStorage[1] = "";
    std::size_t zeroSize = 0;
    cms::StringBuffer zeroOutput(zeroStorage, sizeof(zeroStorage), zeroSize);
    checkResult(
        cms::string::copyTruncated(cms::StringView("x"), zeroOutput),
        cms::Status::no_space,
        0,
        1);
    checkBuffer(zeroOutput, nullptr, 0);

    char appendTruncatedStorage[6] = "ab";
    std::size_t appendTruncatedSize = 2;
    cms::StringBuffer appendTruncatedOutput(
        appendTruncatedStorage,
        sizeof(appendTruncatedStorage),
        appendTruncatedSize);
    checkResult(
        cms::string::appendTruncated(
            cms::StringView("cd"), appendTruncatedOutput),
        cms::Status::ok,
        2,
        2);
    checkBuffer(appendTruncatedOutput, "abcd", 4);
    checkResult(
        cms::string::appendTruncated(
            cms::StringView("XYZ"), appendTruncatedOutput),
        cms::Status::no_space,
        1,
        3);
    checkBuffer(appendTruncatedOutput, "abcdX", 5);
    checkResult(
        cms::string::appendTruncated(
            cms::StringView("z"), appendTruncatedOutput),
        cms::Status::no_space,
        0,
        1);
    checkBuffer(appendTruncatedOutput, "abcdX", 5);

    char aliasStorage[16] = "abcdef";
    std::size_t aliasSize = 6;
    cms::StringBuffer aliasOutput(aliasStorage, sizeof(aliasStorage), aliasSize);
    checkResult(
        cms::string::copy(aliasOutput.view(), aliasOutput),
        cms::Status::ok,
        6,
        6);
    checkBuffer(aliasOutput, "abcdef", 6);
    const cms::StringView partialCopy(aliasOutput.data() + 2, 3);
    checkResult(
        cms::string::copy(partialCopy, aliasOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "cde", 3);

    checkResult(
        cms::string::copy(cms::StringView("abc"), aliasOutput),
        cms::Status::ok,
        3,
        3);
    checkResult(
        cms::string::append(aliasOutput.view(), aliasOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "abcabc", 6);
    checkResult(
        cms::string::copy(cms::StringView("abcdef"), aliasOutput),
        cms::Status::ok,
        6,
        6);
    const cms::StringView partialAppend(aliasOutput.data() + 1, 3);
    checkResult(
        cms::string::append(partialAppend, aliasOutput),
        cms::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "abcdefbcd", 9);

    char smallAliasStorage[8] = "abcdefg";
    std::size_t smallAliasSize = 7;
    cms::StringBuffer smallAlias(
        smallAliasStorage, sizeof(smallAliasStorage), smallAliasSize);
    const cms::StringView wholeStorage(
        smallAlias.data(), smallAlias.capacity());
    checkResult(
        cms::string::copyTruncated(wholeStorage, smallAlias),
        cms::Status::no_space,
        7,
        8);
    checkBuffer(smallAlias, "abcdefg", 7);
    checkResult(
        cms::string::copy(cms::StringView("abcde"), smallAlias),
        cms::Status::ok,
        5,
        5);
    checkResult(
        cms::string::appendTruncated(smallAlias.view(), smallAlias),
        cms::Status::no_space,
        2,
        5);
    checkBuffer(smallAlias, "abcdeab", 7);

    checkReplace(
        cms::StringView("abc"),
        cms::StringView("x"),
        cms::StringView("y"),
        "abc",
        3);
    checkReplace(
        cms::StringView("aba"),
        cms::StringView("b"),
        cms::StringView("X"),
        "aXa",
        3);
    checkReplace(
        cms::StringView("foo foo"),
        cms::StringView("foo"),
        cms::StringView("x"),
        "x x",
        3);
    checkReplace(
        cms::StringView("aaaa"),
        cms::StringView("aa"),
        cms::StringView("b"),
        "bb",
        2);
    checkReplace(
        cms::StringView("aaa"),
        cms::StringView("aa"),
        cms::StringView("b"),
        "ba",
        2);
    checkReplace(
        cms::StringView("abcabc"),
        cms::StringView("abc"),
        cms::StringView("x"),
        "xx",
        2);
    checkReplace(
        cms::StringView("abc"),
        cms::StringView("b"),
        cms::StringView("X"),
        "aXc",
        3);
    checkReplace(
        cms::StringView("abc"),
        cms::StringView("b"),
        cms::StringView("XYZ"),
        "aXYZc",
        5);
    checkReplace(
        cms::StringView("abc"),
        cms::StringView("b"),
        empty,
        "ac",
        2);

    const char nulInput[] = {'A', '\0', 'B', 'A', '\0', 'B'};
    const char nulNeedle[] = {'\0', 'B'};
    checkReplace(
        cms::StringView(nulInput, sizeof(nulInput)),
        cms::StringView(nulNeedle, sizeof(nulNeedle)),
        cms::StringView("X"),
        "AXAX",
        4);
    const char nulReplacement[] = {'X', '\0'};
    const char nulReplacementExpected[] = {'X', '\0', 'X', '\0'};
    checkReplace(
        cms::StringView("aa"),
        cms::StringView("a"),
        cms::StringView(nulReplacement, sizeof(nulReplacement)),
        nulReplacementExpected,
        sizeof(nulReplacementExpected));

    char exactReplaceStorage[6] = "old";
    std::size_t exactReplaceSize = 3;
    cms::StringBuffer exactReplaceOutput(
        exactReplaceStorage,
        sizeof(exactReplaceStorage),
        exactReplaceSize);
    checkResult(
        cms::string::replaceAll(
            cms::StringView("abc"),
            cms::StringView("b"),
            cms::StringView("XYZ"),
            exactReplaceOutput),
        cms::Status::ok,
        5,
        5);
    checkBuffer(exactReplaceOutput, "aXYZc", 5);

    char shortReplaceStorage[5] = "old";
    std::size_t shortReplaceSize = 3;
    cms::StringBuffer shortReplaceOutput(
        shortReplaceStorage,
        sizeof(shortReplaceStorage),
        shortReplaceSize);
    checkResult(
        cms::string::replaceAll(
            cms::StringView("abc"),
            cms::StringView("b"),
            cms::StringView("XYZ"),
            shortReplaceOutput),
        cms::Status::no_space,
        0,
        5);
    checkBuffer(shortReplaceOutput, "old", 3);

    checkResult(
        cms::string::replaceAll(
            cms::StringView("abc"),
            empty,
            cms::StringView("x"),
            shortReplaceOutput),
        cms::Status::invalid_argument,
        0,
        0);
    checkBuffer(shortReplaceOutput, "old", 3);
    checkResult(
        cms::string::replaceAll(
            cms::StringView("abc"),
            cms::StringView("a"),
            cms::StringView("x"),
            defaultOutput),
        cms::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::string::replaceAll(
            cms::StringView("abc"),
            cms::StringView("a"),
            cms::StringView("x"),
            damagedOutput),
        cms::Status::invalid_argument,
        0,
        0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');
    CMS_TEST_CHECK(damagedStorage[1] == 'l');
    CMS_TEST_CHECK(damagedStorage[2] == 'd');
    CMS_TEST_CHECK(damagedStorage[3] == '\0');

    std::printf("cms::string runtime coverage complete\n");
    return cms::test::finish();
}
