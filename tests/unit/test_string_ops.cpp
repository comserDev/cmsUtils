#include <cstdio>

#include <cms/util/string_ops.h>

#include "test.h"

namespace {

constexpr char byte(unsigned int value) noexcept {
    return static_cast<char>(value);
}

void checkResult(
    const cms::util::WriteResult& result,
    cms::util::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

void checkBytes(
    cms::util::StringView actual,
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
    cms::util::StringBuffer output,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(output.valid());
    CMS_TEST_CHECK(output.size() == expectedSize);
    CMS_TEST_REQUIRE(output.data() != nullptr);
    CMS_TEST_CHECK(output.data()[output.size()] == '\0');
    checkBytes(output.view(), expected, expectedSize);
}

void checkReplace(
    cms::util::StringView input,
    cms::util::StringView needle,
    cms::util::StringView replacement,
    const char* expected,
    std::size_t expectedSize) {
    char storage[64] = "old";
    std::size_t size = 3;
    cms::util::StringBuffer output(storage, sizeof(storage), size);
    const cms::util::WriteResult result =
        cms::util::string::replaceAll(input, needle, replacement, output);
    checkResult(result, cms::util::Status::ok, expectedSize, expectedSize);
    checkBuffer(output, expected, expectedSize);
}

} // namespace

int main() {
    const cms::util::StringView empty;
    CMS_TEST_CHECK(cms::util::string::compare(empty, empty) == 0);
    CMS_TEST_CHECK(cms::util::string::compare(empty, cms::util::StringView("a")) == -1);
    CMS_TEST_CHECK(cms::util::string::compare(cms::util::StringView("a"), empty) == 1);
    CMS_TEST_CHECK(
        cms::util::string::compare(cms::util::StringView("abc"), cms::util::StringView("abc"))
        == 0);
    CMS_TEST_CHECK(
        cms::util::string::compare(cms::util::StringView("ab"), cms::util::StringView("abc"))
        == -1);
    CMS_TEST_CHECK(
        cms::util::string::compare(cms::util::StringView("abc"), cms::util::StringView("ab"))
        == 1);
    CMS_TEST_CHECK(
        cms::util::string::compare(cms::util::StringView("abc"), cms::util::StringView("abd"))
        == -1);
    CMS_TEST_CHECK(
        cms::util::string::compare(cms::util::StringView("abd"), cms::util::StringView("abc"))
        == 1);

    const char embeddedLeft[] = {'A', '\0', 'B'};
    const char embeddedRight[] = {'A', '\0', 'C'};
    CMS_TEST_CHECK(
        cms::util::string::compare(
            cms::util::StringView(embeddedLeft, sizeof(embeddedLeft)),
            cms::util::StringView(embeddedRight, sizeof(embeddedRight))) == -1);
    const char lowerUnsigned[] = {byte(0x7F)};
    const char higherUnsigned[] = {byte(0x80)};
    CMS_TEST_CHECK(
        cms::util::string::compare(
            cms::util::StringView(lowerUnsigned, sizeof(lowerUnsigned)),
            cms::util::StringView(higherUnsigned, sizeof(higherUnsigned))) == -1);
    CMS_TEST_CHECK(
        cms::util::string::compare(
            cms::util::StringView(higherUnsigned, sizeof(higherUnsigned)),
            cms::util::StringView(lowerUnsigned, sizeof(lowerUnsigned))) == 1);

    CMS_TEST_CHECK(cms::util::string::equals(empty, empty));
    CMS_TEST_CHECK(
        cms::util::string::equals(cms::util::StringView("abc"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(
        !cms::util::string::equals(cms::util::StringView("ab"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(
        !cms::util::string::equals(cms::util::StringView("abc"), cms::util::StringView("abd")));
    CMS_TEST_CHECK(cms::util::string::equals(
        cms::util::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::util::StringView(embeddedLeft, sizeof(embeddedLeft))));

    CMS_TEST_CHECK(cms::util::string::startsWith(cms::util::StringView("abc"), empty));
    CMS_TEST_CHECK(cms::util::string::startsWith(
        cms::util::StringView("abc"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(cms::util::string::startsWith(
        cms::util::StringView("abc"), cms::util::StringView("ab")));
    CMS_TEST_CHECK(!cms::util::string::startsWith(
        cms::util::StringView("ab"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(!cms::util::string::startsWith(
        cms::util::StringView("abc"), cms::util::StringView("bc")));
    const char embeddedPrefix[] = {'A', '\0'};
    CMS_TEST_CHECK(cms::util::string::startsWith(
        cms::util::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::util::StringView(embeddedPrefix, sizeof(embeddedPrefix))));

    CMS_TEST_CHECK(cms::util::string::endsWith(cms::util::StringView("abc"), empty));
    CMS_TEST_CHECK(cms::util::string::endsWith(
        cms::util::StringView("abc"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(cms::util::string::endsWith(
        cms::util::StringView("abc"), cms::util::StringView("bc")));
    CMS_TEST_CHECK(!cms::util::string::endsWith(
        cms::util::StringView("ab"), cms::util::StringView("abc")));
    CMS_TEST_CHECK(!cms::util::string::endsWith(
        cms::util::StringView("abc"), cms::util::StringView("ab")));
    const char embeddedSuffix[] = {'\0', 'B'};
    CMS_TEST_CHECK(cms::util::string::endsWith(
        cms::util::StringView(embeddedLeft, sizeof(embeddedLeft)),
        cms::util::StringView(embeddedSuffix, sizeof(embeddedSuffix))));

    CMS_TEST_CHECK(cms::util::string::find(empty, empty) == 0);
    CMS_TEST_CHECK(
        cms::util::string::find(empty, cms::util::StringView("a")) == cms::util::string::npos);
    CMS_TEST_CHECK(cms::util::string::find(cms::util::StringView("abc"), empty, 0) == 0);
    CMS_TEST_CHECK(cms::util::string::find(cms::util::StringView("abc"), empty, 2) == 2);
    CMS_TEST_CHECK(cms::util::string::find(cms::util::StringView("abc"), empty, 3) == 3);
    CMS_TEST_CHECK(
        cms::util::string::find(cms::util::StringView("abc"), empty, 4)
        == cms::util::string::npos);
    CMS_TEST_CHECK(
        cms::util::string::find(
            cms::util::StringView("abc"),
            cms::util::StringView("a"),
            cms::util::string::npos) == cms::util::string::npos);
    CMS_TEST_CHECK(
        cms::util::string::find(
            cms::util::StringView("abc"),
            empty,
            cms::util::string::npos) == cms::util::string::npos);
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView("abc"), cms::util::StringView("abc")) == 0);
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView("abcabc"), cms::util::StringView("ab")) == 0);
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView("abcabc"), cms::util::StringView("ca")) == 2);
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView("abcabc"), cms::util::StringView("bc"), 3) == 4);
    CMS_TEST_CHECK(
        cms::util::string::find(cms::util::StringView("abc"), cms::util::StringView("x"))
        == cms::util::string::npos);
    CMS_TEST_CHECK(
        cms::util::string::find(cms::util::StringView("abc"), cms::util::StringView("a"), 3)
        == cms::util::string::npos);
    CMS_TEST_CHECK(
        cms::util::string::find(cms::util::StringView("abc"), cms::util::StringView("a"), 4)
        == cms::util::string::npos);
    CMS_TEST_CHECK(
        cms::util::string::find(cms::util::StringView("ab"), cms::util::StringView("abc"))
        == cms::util::string::npos);
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView("aaa"), cms::util::StringView("aa")) == 0);

    const char embeddedHaystack[] = {'A', '\0', 'B', '\0', 'B'};
    CMS_TEST_CHECK(cms::util::string::find(
        cms::util::StringView(embeddedHaystack, sizeof(embeddedHaystack)),
        cms::util::StringView(embeddedSuffix, sizeof(embeddedSuffix))) == 1);

    CMS_TEST_CHECK(cms::util::string::findLast(empty, empty) == 0);
    CMS_TEST_CHECK(
        cms::util::string::findLast(empty, cms::util::StringView("a"))
        == cms::util::string::npos);
    CMS_TEST_CHECK(cms::util::string::findLast(
        cms::util::StringView("abc"), empty) == 3);
    CMS_TEST_CHECK(cms::util::string::findLast(
        cms::util::StringView("abc"), cms::util::StringView("b")) == 1);
    CMS_TEST_CHECK(cms::util::string::findLast(
        cms::util::StringView("ababa"), cms::util::StringView("ba")) == 3);
    CMS_TEST_CHECK(cms::util::string::findLast(
        cms::util::StringView("aaa"), cms::util::StringView("aa")) == 1);
    CMS_TEST_CHECK(
        cms::util::string::findLast(cms::util::StringView("abc"), cms::util::StringView("x"))
        == cms::util::string::npos);
    CMS_TEST_CHECK(cms::util::string::findLast(
        cms::util::StringView(embeddedHaystack, sizeof(embeddedHaystack)),
        cms::util::StringView(embeddedSuffix, sizeof(embeddedSuffix))) == 3);

    char copyStorage[8] = "old";
    std::size_t copySize = 3;
    cms::util::StringBuffer copyOutput(copyStorage, sizeof(copyStorage), copySize);
    checkResult(
        cms::util::string::copy(empty, copyOutput),
        cms::util::Status::ok,
        0,
        0);
    checkBuffer(copyOutput, nullptr, 0);
    checkResult(
        cms::util::string::copy(cms::util::StringView("abc"), copyOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(copyOutput, "abc", 3);
    checkResult(
        cms::util::string::copy(cms::util::StringView("xy"), copyOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(copyOutput, "xy", 2);
    checkResult(
        cms::util::string::copy(cms::util::StringView("1234567"), copyOutput),
        cms::util::Status::ok,
        7,
        7);
    checkBuffer(copyOutput, "1234567", 7);

    const char tooLarge[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    checkResult(
        cms::util::string::copy(
            cms::util::StringView(tooLarge, sizeof(tooLarge)),
            copyOutput),
        cms::util::Status::no_space,
        0,
        sizeof(tooLarge));
    checkBuffer(copyOutput, "1234567", 7);

    const char embeddedInput[] = {'A', '\0', 'B'};
    checkResult(
        cms::util::string::copy(
            cms::util::StringView(embeddedInput, sizeof(embeddedInput)),
            copyOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(copyOutput, embeddedInput, sizeof(embeddedInput));

    cms::util::StringBuffer defaultOutput;
    checkResult(
        cms::util::string::copy(cms::util::StringView("x"), defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    char damagedStorage[8] = "old";
    std::size_t damagedSize = 3;
    cms::util::StringBuffer damagedOutput(
        damagedStorage, sizeof(damagedStorage), damagedSize);
    damagedSize = sizeof(damagedStorage);
    checkResult(
        cms::util::string::copy(cms::util::StringView("x"), damagedOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');
    CMS_TEST_CHECK(damagedStorage[1] == 'l');
    CMS_TEST_CHECK(damagedStorage[2] == 'd');
    CMS_TEST_CHECK(damagedStorage[3] == '\0');

    char appendStorage[8] = "";
    std::size_t appendSize = 0;
    cms::util::StringBuffer appendOutput(
        appendStorage, sizeof(appendStorage), appendSize);
    checkResult(
        cms::util::string::append(cms::util::StringView("ab"), appendOutput),
        cms::util::Status::ok,
        2,
        2);
    checkResult(
        cms::util::string::append(cms::util::StringView("cd"), appendOutput),
        cms::util::Status::ok,
        2,
        2);
    checkResult(
        cms::util::string::append(cms::util::StringView("efg"), appendOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::util::string::append(cms::util::StringView("x"), appendOutput),
        cms::util::Status::no_space,
        0,
        1);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::util::string::append(empty, appendOutput),
        cms::util::Status::ok,
        0,
        0);
    checkBuffer(appendOutput, "abcdefg", 7);
    checkResult(
        cms::util::string::append(cms::util::StringView("x"), defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);

    char embeddedAppendStorage[8] = "X";
    std::size_t embeddedAppendSize = 1;
    cms::util::StringBuffer embeddedAppend(
        embeddedAppendStorage,
        sizeof(embeddedAppendStorage),
        embeddedAppendSize);
    checkResult(
        cms::util::string::append(
            cms::util::StringView(embeddedInput, sizeof(embeddedInput)),
            embeddedAppend),
        cms::util::Status::ok,
        3,
        3);
    const char embeddedAppended[] = {'X', 'A', '\0', 'B'};
    checkBuffer(
        embeddedAppend,
        embeddedAppended,
        sizeof(embeddedAppended));

    char truncatedStorage[4] = "old";
    std::size_t truncatedSize = 3;
    cms::util::StringBuffer truncatedOutput(
        truncatedStorage, sizeof(truncatedStorage), truncatedSize);
    checkResult(
        cms::util::string::copyTruncated(cms::util::StringView("xy"), truncatedOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(truncatedOutput, "xy", 2);
    checkResult(
        cms::util::string::copyTruncated(cms::util::StringView("abcde"), truncatedOutput),
        cms::util::Status::no_space,
        3,
        5);
    checkBuffer(truncatedOutput, "abc", 3);

    char zeroStorage[1] = "";
    std::size_t zeroSize = 0;
    cms::util::StringBuffer zeroOutput(zeroStorage, sizeof(zeroStorage), zeroSize);
    checkResult(
        cms::util::string::copyTruncated(cms::util::StringView("x"), zeroOutput),
        cms::util::Status::no_space,
        0,
        1);
    checkBuffer(zeroOutput, nullptr, 0);

    char appendTruncatedStorage[6] = "ab";
    std::size_t appendTruncatedSize = 2;
    cms::util::StringBuffer appendTruncatedOutput(
        appendTruncatedStorage,
        sizeof(appendTruncatedStorage),
        appendTruncatedSize);
    checkResult(
        cms::util::string::appendTruncated(
            cms::util::StringView("cd"), appendTruncatedOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(appendTruncatedOutput, "abcd", 4);
    checkResult(
        cms::util::string::appendTruncated(
            cms::util::StringView("XYZ"), appendTruncatedOutput),
        cms::util::Status::no_space,
        1,
        3);
    checkBuffer(appendTruncatedOutput, "abcdX", 5);
    checkResult(
        cms::util::string::appendTruncated(
            cms::util::StringView("z"), appendTruncatedOutput),
        cms::util::Status::no_space,
        0,
        1);
    checkBuffer(appendTruncatedOutput, "abcdX", 5);

    char aliasStorage[16] = "abcdef";
    std::size_t aliasSize = 6;
    cms::util::StringBuffer aliasOutput(aliasStorage, sizeof(aliasStorage), aliasSize);
    checkResult(
        cms::util::string::copy(aliasOutput.view(), aliasOutput),
        cms::util::Status::ok,
        6,
        6);
    checkBuffer(aliasOutput, "abcdef", 6);
    const cms::util::StringView partialCopy(aliasOutput.data() + 2, 3);
    checkResult(
        cms::util::string::copy(partialCopy, aliasOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "cde", 3);

    checkResult(
        cms::util::string::copy(cms::util::StringView("abc"), aliasOutput),
        cms::util::Status::ok,
        3,
        3);
    checkResult(
        cms::util::string::append(aliasOutput.view(), aliasOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "abcabc", 6);
    checkResult(
        cms::util::string::copy(cms::util::StringView("abcdef"), aliasOutput),
        cms::util::Status::ok,
        6,
        6);
    const cms::util::StringView partialAppend(aliasOutput.data() + 1, 3);
    checkResult(
        cms::util::string::append(partialAppend, aliasOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(aliasOutput, "abcdefbcd", 9);

    char smallAliasStorage[8] = "abcdefg";
    std::size_t smallAliasSize = 7;
    cms::util::StringBuffer smallAlias(
        smallAliasStorage, sizeof(smallAliasStorage), smallAliasSize);
    const cms::util::StringView wholeStorage(
        smallAlias.data(), smallAlias.capacity());
    checkResult(
        cms::util::string::copyTruncated(wholeStorage, smallAlias),
        cms::util::Status::no_space,
        7,
        8);
    checkBuffer(smallAlias, "abcdefg", 7);
    checkResult(
        cms::util::string::copy(cms::util::StringView("abcde"), smallAlias),
        cms::util::Status::ok,
        5,
        5);
    checkResult(
        cms::util::string::appendTruncated(smallAlias.view(), smallAlias),
        cms::util::Status::no_space,
        2,
        5);
    checkBuffer(smallAlias, "abcdeab", 7);

    checkReplace(
        cms::util::StringView("abc"),
        cms::util::StringView("x"),
        cms::util::StringView("y"),
        "abc",
        3);
    checkReplace(
        cms::util::StringView("aba"),
        cms::util::StringView("b"),
        cms::util::StringView("X"),
        "aXa",
        3);
    checkReplace(
        cms::util::StringView("foo foo"),
        cms::util::StringView("foo"),
        cms::util::StringView("x"),
        "x x",
        3);
    checkReplace(
        cms::util::StringView("aaaa"),
        cms::util::StringView("aa"),
        cms::util::StringView("b"),
        "bb",
        2);
    checkReplace(
        cms::util::StringView("aaa"),
        cms::util::StringView("aa"),
        cms::util::StringView("b"),
        "ba",
        2);
    checkReplace(
        cms::util::StringView("abcabc"),
        cms::util::StringView("abc"),
        cms::util::StringView("x"),
        "xx",
        2);
    checkReplace(
        cms::util::StringView("abc"),
        cms::util::StringView("b"),
        cms::util::StringView("X"),
        "aXc",
        3);
    checkReplace(
        cms::util::StringView("abc"),
        cms::util::StringView("b"),
        cms::util::StringView("XYZ"),
        "aXYZc",
        5);
    checkReplace(
        cms::util::StringView("abc"),
        cms::util::StringView("b"),
        empty,
        "ac",
        2);

    const char nulInput[] = {'A', '\0', 'B', 'A', '\0', 'B'};
    const char nulNeedle[] = {'\0', 'B'};
    checkReplace(
        cms::util::StringView(nulInput, sizeof(nulInput)),
        cms::util::StringView(nulNeedle, sizeof(nulNeedle)),
        cms::util::StringView("X"),
        "AXAX",
        4);
    const char nulReplacement[] = {'X', '\0'};
    const char nulReplacementExpected[] = {'X', '\0', 'X', '\0'};
    checkReplace(
        cms::util::StringView("aa"),
        cms::util::StringView("a"),
        cms::util::StringView(nulReplacement, sizeof(nulReplacement)),
        nulReplacementExpected,
        sizeof(nulReplacementExpected));

    char exactReplaceStorage[6] = "old";
    std::size_t exactReplaceSize = 3;
    cms::util::StringBuffer exactReplaceOutput(
        exactReplaceStorage,
        sizeof(exactReplaceStorage),
        exactReplaceSize);
    checkResult(
        cms::util::string::replaceAll(
            cms::util::StringView("abc"),
            cms::util::StringView("b"),
            cms::util::StringView("XYZ"),
            exactReplaceOutput),
        cms::util::Status::ok,
        5,
        5);
    checkBuffer(exactReplaceOutput, "aXYZc", 5);

    char shortReplaceStorage[5] = "old";
    std::size_t shortReplaceSize = 3;
    cms::util::StringBuffer shortReplaceOutput(
        shortReplaceStorage,
        sizeof(shortReplaceStorage),
        shortReplaceSize);
    checkResult(
        cms::util::string::replaceAll(
            cms::util::StringView("abc"),
            cms::util::StringView("b"),
            cms::util::StringView("XYZ"),
            shortReplaceOutput),
        cms::util::Status::no_space,
        0,
        5);
    checkBuffer(shortReplaceOutput, "old", 3);

    checkResult(
        cms::util::string::replaceAll(
            cms::util::StringView("abc"),
            empty,
            cms::util::StringView("x"),
            shortReplaceOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkBuffer(shortReplaceOutput, "old", 3);
    checkResult(
        cms::util::string::replaceAll(
            cms::util::StringView("abc"),
            cms::util::StringView("a"),
            cms::util::StringView("x"),
            defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::string::replaceAll(
            cms::util::StringView("abc"),
            cms::util::StringView("a"),
            cms::util::StringView("x"),
            damagedOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');
    CMS_TEST_CHECK(damagedStorage[1] == 'l');
    CMS_TEST_CHECK(damagedStorage[2] == 'd');
    CMS_TEST_CHECK(damagedStorage[3] == '\0');

    std::printf("cms::util::string runtime coverage complete\n");
    return cms::test::finish();
}
