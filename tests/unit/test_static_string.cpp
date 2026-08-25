#include <cstdio>
#include <utility>

#include <cms/static_string.h>

#include "test.h"

namespace {

template<std::size_t StorageBytes>
void checkInvariant(const cms::StaticString<StorageBytes>& value) {
    CMS_TEST_REQUIRE(value.cStr() != nullptr);
    CMS_TEST_REQUIRE(value.data() != nullptr);
    CMS_TEST_REQUIRE(value.size() < value.capacity());
    CMS_TEST_CHECK(value.data() == value.cStr());
    CMS_TEST_CHECK(value.capacity() == StorageBytes);
    CMS_TEST_CHECK(value.maxSize() == StorageBytes - 1);
    CMS_TEST_CHECK(value.remaining() == value.maxSize() - value.size());
    CMS_TEST_CHECK(value.cStr()[value.size()] == '\0');
    CMS_TEST_CHECK(value.empty() == (value.size() == 0));
}

template<std::size_t StorageBytes>
void checkContent(
    const cms::StaticString<StorageBytes>& value,
    const char* expected,
    std::size_t expectedSize) {
    checkInvariant(value);
    CMS_TEST_REQUIRE(value.size() == expectedSize);
    if (expectedSize > 0) {
        CMS_TEST_REQUIRE(expected != nullptr);
    }

    for (std::size_t index = 0; index < expectedSize; ++index) {
        CMS_TEST_CHECK(value.view()[index] == expected[index]);
    }
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

} // namespace

int main() {
    cms::StaticString<8> basic;
    checkContent(basic, nullptr, 0);
    CMS_TEST_CHECK(basic.capacity() == 8);
    CMS_TEST_CHECK(basic.maxSize() == 7);

    cms::StaticString<1> tiny;
    checkContent(tiny, nullptr, 0);
    CMS_TEST_CHECK(tiny.capacity() == 1);
    CMS_TEST_CHECK(tiny.maxSize() == 0);
    checkResult(
        tiny.assign(cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.assignTruncated(cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.append(cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.appendTruncated(cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);

    checkResult(
        basic.assign(cms::StringView("old")),
        cms::Status::ok,
        3,
        3);
    basic.clear();
    checkContent(basic, nullptr, 0);

    checkResult(
        basic.assign(cms::StringView("old")),
        cms::Status::ok,
        3,
        3);
    checkResult(
        basic.assign(cms::StringView()),
        cms::Status::ok,
        0,
        0);
    checkContent(basic, nullptr, 0);

    const char exactPayload[] = {'1', '2', '3', '4', '5', '6', '7'};
    checkResult(
        basic.assign(cms::StringView(exactPayload, sizeof(exactPayload))),
        cms::Status::ok,
        sizeof(exactPayload),
        sizeof(exactPayload));
    checkContent(basic, exactPayload, sizeof(exactPayload));

    const char tooLarge[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    checkResult(
        basic.assign(cms::StringView(tooLarge, sizeof(tooLarge))),
        cms::Status::no_space,
        0,
        sizeof(tooLarge));
    checkContent(basic, exactPayload, sizeof(exactPayload));

    checkResult(
        basic.assign(cms::StringView("keep")),
        cms::Status::ok,
        4,
        4);
    checkResult(
        basic.assign(cms::StringView(tooLarge, sizeof(tooLarge))),
        cms::Status::no_space,
        0,
        sizeof(tooLarge));
    checkContent(basic, "keep", 4);
    checkResult(
        basic.assign(cms::StringView("new")),
        cms::Status::ok,
        3,
        3);
    checkContent(basic, "new", 3);

    const char embeddedNul[] = {'A', '\0', 'B'};
    checkResult(
        basic.assign(cms::StringView(embeddedNul, sizeof(embeddedNul))),
        cms::Status::ok,
        sizeof(embeddedNul),
        sizeof(embeddedNul));
    checkContent(basic, embeddedNul, sizeof(embeddedNul));

    cms::StaticString<8> appended;
    checkResult(
        appended.append(cms::StringView("ab")),
        cms::Status::ok,
        2,
        2);
    checkContent(appended, "ab", 2);
    checkResult(
        appended.append(cms::StringView("cd")),
        cms::Status::ok,
        2,
        2);
    checkContent(appended, "abcd", 4);
    checkResult(
        appended.append(cms::StringView("efg")),
        cms::Status::ok,
        3,
        3);
    checkContent(appended, "abcdefg", 7);
    checkResult(
        appended.append(cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    checkContent(appended, "abcdefg", 7);
    checkResult(
        appended.append(cms::StringView()),
        cms::Status::ok,
        0,
        0);
    checkContent(appended, "abcdefg", 7);

    cms::StaticString<8> truncated;
    checkResult(
        truncated.assignTruncated(cms::StringView("fit")),
        cms::Status::ok,
        3,
        3);
    checkContent(truncated, "fit", 3);
    const char longPayload[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};
    checkResult(
        truncated.assignTruncated(
            cms::StringView(longPayload, sizeof(longPayload))),
        cms::Status::no_space,
        7,
        sizeof(longPayload));
    checkContent(truncated, "abcdefg", 7);

    truncated.clear();
    checkResult(
        truncated.appendTruncated(cms::StringView("ab")),
        cms::Status::ok,
        2,
        2);
    checkResult(
        truncated.appendTruncated(cms::StringView("cd")),
        cms::Status::ok,
        2,
        2);
    checkContent(truncated, "abcd", 4);
    checkResult(
        truncated.appendTruncated(cms::StringView("WXYZ")),
        cms::Status::no_space,
        3,
        4);
    checkContent(truncated, "abcdWXY", 7);
    checkResult(
        truncated.appendTruncated(cms::StringView("z")),
        cms::Status::no_space,
        0,
        1);
    checkContent(truncated, "abcdWXY", 7);
    checkResult(
        truncated.appendTruncated(cms::StringView()),
        cms::Status::ok,
        0,
        0);
    checkContent(truncated, "abcdWXY", 7);

    cms::StaticString<16> copySource;
    checkResult(
        copySource.assign(cms::StringView("abc")),
        cms::Status::ok,
        3,
        3);
    cms::StaticString<16> copyConstructed(copySource);
    CMS_TEST_CHECK(copyConstructed.cStr() != copySource.cStr());
    checkContent(copyConstructed, "abc", 3);
    copyConstructed.clear();
    checkContent(copySource, "abc", 3);

    cms::StaticString<16> copyAssigned;
    cms::StringBuffer copyDestinationAlias = copyAssigned.buffer();
    const char* copyDestinationStorage = copyAssigned.cStr();
    copyAssigned = copySource;
    CMS_TEST_CHECK(copyAssigned.cStr() != copySource.cStr());
    checkContent(copyAssigned, "abc", 3);
    CMS_TEST_CHECK(copyDestinationAlias.data() == copyDestinationStorage);
    CMS_TEST_CHECK(copyDestinationAlias.data() == copyAssigned.cStr());
    CMS_TEST_CHECK(copyDestinationAlias.size() == copyAssigned.size());
    CMS_TEST_CHECK(copyDestinationAlias.size() == 3);
    CMS_TEST_CHECK(copyDestinationAlias.valid());
    checkResult(
        copyAssigned.assign(cms::StringView("xyz")),
        cms::Status::ok,
        3,
        3);
    checkContent(copySource, "abc", 3);

    cms::StaticString<16> moveSource;
    checkResult(
        moveSource.assign(cms::StringView("move")),
        cms::Status::ok,
        4,
        4);
    cms::StringBuffer oldMoveBuffer = moveSource.buffer();
    const char* oldMoveStorage = moveSource.cStr();
    cms::StaticString<16> moveConstructed(std::move(moveSource));
    CMS_TEST_CHECK(moveConstructed.cStr() != oldMoveStorage);
    checkContent(moveConstructed, "move", 4);
    checkContent(moveSource, nullptr, 0);
    CMS_TEST_CHECK(oldMoveBuffer.data() == oldMoveStorage);
    CMS_TEST_CHECK(oldMoveBuffer.size() == 0);

    cms::StaticString<16> moveAssignSource;
    checkResult(
        moveAssignSource.assign(cms::StringView("move")),
        cms::Status::ok,
        4,
        4);
    cms::StringBuffer moveSourceAlias = moveAssignSource.buffer();
    const char* moveAssignSourceStorage = moveAssignSource.cStr();
    cms::StaticString<16> moveAssigned;
    cms::StringBuffer moveDestinationAlias = moveAssigned.buffer();
    const char* moveDestinationStorage = moveAssigned.cStr();
    checkResult(
        moveAssigned.assign(cms::StringView("old")),
        cms::Status::ok,
        3,
        3);
    moveAssigned = std::move(moveAssignSource);
    CMS_TEST_CHECK(moveAssigned.cStr() != moveAssignSourceStorage);
    checkContent(moveAssigned, "move", 4);
    checkContent(moveAssignSource, nullptr, 0);
    CMS_TEST_CHECK(moveDestinationAlias.data() == moveDestinationStorage);
    CMS_TEST_CHECK(moveDestinationAlias.data() == moveAssigned.cStr());
    CMS_TEST_CHECK(moveDestinationAlias.size() == moveAssigned.size());
    CMS_TEST_CHECK(moveDestinationAlias.valid());
    CMS_TEST_CHECK(moveSourceAlias.data() == moveAssignSourceStorage);
    CMS_TEST_CHECK(moveSourceAlias.data() == moveAssignSource.cStr());
    CMS_TEST_CHECK(moveSourceAlias.size() == 0);
    CMS_TEST_CHECK(moveSourceAlias.valid());

    const char* selfMoveStorage = moveAssigned.cStr();
    moveAssigned = std::move(moveAssigned);
    CMS_TEST_CHECK(moveAssigned.cStr() == selfMoveStorage);
    checkContent(moveAssigned, "move", 4);

    cms::StaticString<16> viewed;
    checkResult(
        viewed.assign(cms::StringView("snapshot")),
        cms::Status::ok,
        8,
        8);
    const cms::StringView snapshot = viewed.view();
    CMS_TEST_REQUIRE(snapshot.size() == 8);
    CMS_TEST_CHECK(snapshot.data() == viewed.cStr());
    viewed.clear();
    CMS_TEST_CHECK(snapshot.size() == 8);
    CMS_TEST_CHECK(snapshot.data() == viewed.cStr());
    CMS_TEST_CHECK(snapshot[0] == '\0');
    checkContent(viewed, nullptr, 0);

    cms::StaticString<16> buffered;
    cms::StringBuffer bufferA = buffered.buffer();
    cms::StringBuffer bufferB = buffered.buffer();
    CMS_TEST_REQUIRE(bufferA.valid());
    CMS_TEST_REQUIRE(bufferB.valid());
    bufferA.data()[0] = 'A';
    bufferA.data()[1] = 'B';
    CMS_TEST_CHECK(bufferA.commit(2) == cms::Status::ok);
    CMS_TEST_CHECK(buffered.size() == 2);
    CMS_TEST_CHECK(bufferB.size() == 2);
    checkContent(buffered, "AB", 2);
    CMS_TEST_CHECK(bufferB.clear() == cms::Status::ok);
    CMS_TEST_CHECK(buffered.empty());
    CMS_TEST_CHECK(bufferA.size() == 0);
    checkContent(buffered, nullptr, 0);

    cms::StaticString<16> alias;
    checkResult(
        alias.assign(cms::StringView("abcdef")),
        cms::Status::ok,
        6,
        6);
    checkResult(
        alias.assign(alias.view()),
        cms::Status::ok,
        6,
        6);
    checkContent(alias, "abcdef", 6);
    checkResult(
        alias.append(alias.view()),
        cms::Status::ok,
        6,
        6);
    checkContent(alias, "abcdefabcdef", 12);

    checkResult(
        alias.assign(cms::StringView("abcdef")),
        cms::Status::ok,
        6,
        6);
    const cms::StringView assignPart(alias.cStr() + 2, 3);
    checkResult(alias.assign(assignPart), cms::Status::ok, 3, 3);
    checkContent(alias, "cde", 3);

    checkResult(
        alias.assign(cms::StringView("abcdef")),
        cms::Status::ok,
        6,
        6);
    const cms::StringView appendPart(alias.cStr() + 1, 3);
    checkResult(alias.append(appendPart), cms::Status::ok, 3, 3);
    checkContent(alias, "abcdefbcd", 9);

    cms::StaticString<8> truncatedAlias;
    checkResult(
        truncatedAlias.assign(cms::StringView("abcdefg")),
        cms::Status::ok,
        7,
        7);
    const cms::StringView fullStorage(
        truncatedAlias.cStr(),
        truncatedAlias.capacity());
    checkResult(
        truncatedAlias.assignTruncated(fullStorage),
        cms::Status::no_space,
        7,
        8);
    checkContent(truncatedAlias, "abcdefg", 7);

    checkResult(
        truncatedAlias.assign(cms::StringView("abcde")),
        cms::Status::ok,
        5,
        5);
    const cms::StringView appendSelf = truncatedAlias.view();
    checkResult(
        truncatedAlias.appendTruncated(appendSelf),
        cms::Status::no_space,
        2,
        5);
    checkContent(truncatedAlias, "abcdeab", 7);

    checkResult(
        truncatedAlias.assign(cms::StringView("abcde")),
        cms::Status::ok,
        5,
        5);
    checkResult(
        truncatedAlias.append(truncatedAlias.view()),
        cms::Status::no_space,
        0,
        5);
    checkContent(truncatedAlias, "abcde", 5);

    std::printf(
        "sizeof(cms::StaticString<1>)=%zu\n",
        sizeof(cms::StaticString<1>));
    std::printf(
        "sizeof(cms::StaticString<8>)=%zu\n",
        sizeof(cms::StaticString<8>));
    std::printf(
        "sizeof(cms::StaticString<32>)=%zu\n",
        sizeof(cms::StaticString<32>));
    std::printf(
        "sizeof(cms::StaticString<64>)=%zu\n",
        sizeof(cms::StaticString<64>));
    return cms::test::finish();
}
