#include <cstdio>
#include <utility>

#include <cms/util/static_string.h>

#include "test.h"

namespace {

template<std::size_t StorageBytes>
void checkInvariant(const cms::util::StaticString<StorageBytes>& value) {
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
    const cms::util::StaticString<StorageBytes>& value,
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
    const cms::util::WriteResult& result,
    cms::util::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

} // namespace

int main() {
    cms::util::StaticString<8> basic;
    checkContent(basic, nullptr, 0);
    CMS_TEST_CHECK(basic.capacity() == 8);
    CMS_TEST_CHECK(basic.maxSize() == 7);

    cms::util::StaticString<1> tiny;
    checkContent(tiny, nullptr, 0);
    CMS_TEST_CHECK(tiny.capacity() == 1);
    CMS_TEST_CHECK(tiny.maxSize() == 0);
    checkResult(
        tiny.assign(cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.assignTruncated(cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.append(cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);
    checkResult(
        tiny.appendTruncated(cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(tiny, nullptr, 0);

    checkResult(
        basic.assign(cms::util::StringView("old")),
        cms::util::Status::ok,
        3,
        3);
    basic.clear();
    checkContent(basic, nullptr, 0);

    checkResult(
        basic.assign(cms::util::StringView("old")),
        cms::util::Status::ok,
        3,
        3);
    checkResult(
        basic.assign(cms::util::StringView()),
        cms::util::Status::ok,
        0,
        0);
    checkContent(basic, nullptr, 0);

    const char exactPayload[] = {'1', '2', '3', '4', '5', '6', '7'};
    checkResult(
        basic.assign(cms::util::StringView(exactPayload, sizeof(exactPayload))),
        cms::util::Status::ok,
        sizeof(exactPayload),
        sizeof(exactPayload));
    checkContent(basic, exactPayload, sizeof(exactPayload));

    const char tooLarge[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    checkResult(
        basic.assign(cms::util::StringView(tooLarge, sizeof(tooLarge))),
        cms::util::Status::no_space,
        0,
        sizeof(tooLarge));
    checkContent(basic, exactPayload, sizeof(exactPayload));

    checkResult(
        basic.assign(cms::util::StringView("keep")),
        cms::util::Status::ok,
        4,
        4);
    checkResult(
        basic.assign(cms::util::StringView(tooLarge, sizeof(tooLarge))),
        cms::util::Status::no_space,
        0,
        sizeof(tooLarge));
    checkContent(basic, "keep", 4);
    checkResult(
        basic.assign(cms::util::StringView("new")),
        cms::util::Status::ok,
        3,
        3);
    checkContent(basic, "new", 3);

    const char embeddedNul[] = {'A', '\0', 'B'};
    checkResult(
        basic.assign(cms::util::StringView(embeddedNul, sizeof(embeddedNul))),
        cms::util::Status::ok,
        sizeof(embeddedNul),
        sizeof(embeddedNul));
    checkContent(basic, embeddedNul, sizeof(embeddedNul));

    cms::util::StaticString<8> appended;
    checkResult(
        appended.append(cms::util::StringView("ab")),
        cms::util::Status::ok,
        2,
        2);
    checkContent(appended, "ab", 2);
    checkResult(
        appended.append(cms::util::StringView("cd")),
        cms::util::Status::ok,
        2,
        2);
    checkContent(appended, "abcd", 4);
    checkResult(
        appended.append(cms::util::StringView("efg")),
        cms::util::Status::ok,
        3,
        3);
    checkContent(appended, "abcdefg", 7);
    checkResult(
        appended.append(cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(appended, "abcdefg", 7);
    checkResult(
        appended.append(cms::util::StringView()),
        cms::util::Status::ok,
        0,
        0);
    checkContent(appended, "abcdefg", 7);

    cms::util::StaticString<8> truncated;
    checkResult(
        truncated.assignTruncated(cms::util::StringView("fit")),
        cms::util::Status::ok,
        3,
        3);
    checkContent(truncated, "fit", 3);
    const char longPayload[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};
    checkResult(
        truncated.assignTruncated(
            cms::util::StringView(longPayload, sizeof(longPayload))),
        cms::util::Status::no_space,
        7,
        sizeof(longPayload));
    checkContent(truncated, "abcdefg", 7);

    truncated.clear();
    checkResult(
        truncated.appendTruncated(cms::util::StringView("ab")),
        cms::util::Status::ok,
        2,
        2);
    checkResult(
        truncated.appendTruncated(cms::util::StringView("cd")),
        cms::util::Status::ok,
        2,
        2);
    checkContent(truncated, "abcd", 4);
    checkResult(
        truncated.appendTruncated(cms::util::StringView("WXYZ")),
        cms::util::Status::no_space,
        3,
        4);
    checkContent(truncated, "abcdWXY", 7);
    checkResult(
        truncated.appendTruncated(cms::util::StringView("z")),
        cms::util::Status::no_space,
        0,
        1);
    checkContent(truncated, "abcdWXY", 7);
    checkResult(
        truncated.appendTruncated(cms::util::StringView()),
        cms::util::Status::ok,
        0,
        0);
    checkContent(truncated, "abcdWXY", 7);

    cms::util::StaticString<16> copySource;
    checkResult(
        copySource.assign(cms::util::StringView("abc")),
        cms::util::Status::ok,
        3,
        3);
    cms::util::StaticString<16> copyConstructed(copySource);
    CMS_TEST_CHECK(copyConstructed.cStr() != copySource.cStr());
    checkContent(copyConstructed, "abc", 3);
    copyConstructed.clear();
    checkContent(copySource, "abc", 3);

    cms::util::StaticString<16> copyAssigned;
    cms::util::StringBuffer copyDestinationAlias = copyAssigned.buffer();
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
        copyAssigned.assign(cms::util::StringView("xyz")),
        cms::util::Status::ok,
        3,
        3);
    checkContent(copySource, "abc", 3);

    cms::util::StaticString<16> moveSource;
    checkResult(
        moveSource.assign(cms::util::StringView("move")),
        cms::util::Status::ok,
        4,
        4);
    cms::util::StringBuffer oldMoveBuffer = moveSource.buffer();
    const char* oldMoveStorage = moveSource.cStr();
    cms::util::StaticString<16> moveConstructed(std::move(moveSource));
    CMS_TEST_CHECK(moveConstructed.cStr() != oldMoveStorage);
    checkContent(moveConstructed, "move", 4);
    checkContent(moveSource, nullptr, 0);
    CMS_TEST_CHECK(oldMoveBuffer.data() == oldMoveStorage);
    CMS_TEST_CHECK(oldMoveBuffer.size() == 0);

    cms::util::StaticString<16> moveAssignSource;
    checkResult(
        moveAssignSource.assign(cms::util::StringView("move")),
        cms::util::Status::ok,
        4,
        4);
    cms::util::StringBuffer moveSourceAlias = moveAssignSource.buffer();
    const char* moveAssignSourceStorage = moveAssignSource.cStr();
    cms::util::StaticString<16> moveAssigned;
    cms::util::StringBuffer moveDestinationAlias = moveAssigned.buffer();
    const char* moveDestinationStorage = moveAssigned.cStr();
    checkResult(
        moveAssigned.assign(cms::util::StringView("old")),
        cms::util::Status::ok,
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

    cms::util::StaticString<16> viewed;
    checkResult(
        viewed.assign(cms::util::StringView("snapshot")),
        cms::util::Status::ok,
        8,
        8);
    const cms::util::StringView snapshot = viewed.view();
    CMS_TEST_REQUIRE(snapshot.size() == 8);
    CMS_TEST_CHECK(snapshot.data() == viewed.cStr());
    viewed.clear();
    CMS_TEST_CHECK(snapshot.size() == 8);
    CMS_TEST_CHECK(snapshot.data() == viewed.cStr());
    CMS_TEST_CHECK(snapshot[0] == '\0');
    checkContent(viewed, nullptr, 0);

    cms::util::StaticString<16> buffered;
    cms::util::StringBuffer bufferA = buffered.buffer();
    cms::util::StringBuffer bufferB = buffered.buffer();
    CMS_TEST_REQUIRE(bufferA.valid());
    CMS_TEST_REQUIRE(bufferB.valid());
    bufferA.data()[0] = 'A';
    bufferA.data()[1] = 'B';
    CMS_TEST_CHECK(bufferA.commit(2) == cms::util::Status::ok);
    CMS_TEST_CHECK(buffered.size() == 2);
    CMS_TEST_CHECK(bufferB.size() == 2);
    checkContent(buffered, "AB", 2);
    CMS_TEST_CHECK(bufferB.clear() == cms::util::Status::ok);
    CMS_TEST_CHECK(buffered.empty());
    CMS_TEST_CHECK(bufferA.size() == 0);
    checkContent(buffered, nullptr, 0);

    cms::util::StaticString<16> alias;
    checkResult(
        alias.assign(cms::util::StringView("abcdef")),
        cms::util::Status::ok,
        6,
        6);
    checkResult(
        alias.assign(alias.view()),
        cms::util::Status::ok,
        6,
        6);
    checkContent(alias, "abcdef", 6);
    checkResult(
        alias.append(alias.view()),
        cms::util::Status::ok,
        6,
        6);
    checkContent(alias, "abcdefabcdef", 12);

    checkResult(
        alias.assign(cms::util::StringView("abcdef")),
        cms::util::Status::ok,
        6,
        6);
    const cms::util::StringView assignPart(alias.cStr() + 2, 3);
    checkResult(alias.assign(assignPart), cms::util::Status::ok, 3, 3);
    checkContent(alias, "cde", 3);

    checkResult(
        alias.assign(cms::util::StringView("abcdef")),
        cms::util::Status::ok,
        6,
        6);
    const cms::util::StringView appendPart(alias.cStr() + 1, 3);
    checkResult(alias.append(appendPart), cms::util::Status::ok, 3, 3);
    checkContent(alias, "abcdefbcd", 9);

    cms::util::StaticString<8> truncatedAlias;
    checkResult(
        truncatedAlias.assign(cms::util::StringView("abcdefg")),
        cms::util::Status::ok,
        7,
        7);
    const cms::util::StringView fullStorage(
        truncatedAlias.cStr(),
        truncatedAlias.capacity());
    checkResult(
        truncatedAlias.assignTruncated(fullStorage),
        cms::util::Status::no_space,
        7,
        8);
    checkContent(truncatedAlias, "abcdefg", 7);

    checkResult(
        truncatedAlias.assign(cms::util::StringView("abcde")),
        cms::util::Status::ok,
        5,
        5);
    const cms::util::StringView appendSelf = truncatedAlias.view();
    checkResult(
        truncatedAlias.appendTruncated(appendSelf),
        cms::util::Status::no_space,
        2,
        5);
    checkContent(truncatedAlias, "abcdeab", 7);

    checkResult(
        truncatedAlias.assign(cms::util::StringView("abcde")),
        cms::util::Status::ok,
        5,
        5);
    checkResult(
        truncatedAlias.append(truncatedAlias.view()),
        cms::util::Status::no_space,
        0,
        5);
    checkContent(truncatedAlias, "abcde", 5);

    std::printf(
        "sizeof(cms::util::StaticString<1>)=%zu\n",
        sizeof(cms::util::StaticString<1>));
    std::printf(
        "sizeof(cms::util::StaticString<8>)=%zu\n",
        sizeof(cms::util::StaticString<8>));
    std::printf(
        "sizeof(cms::util::StaticString<32>)=%zu\n",
        sizeof(cms::util::StaticString<32>));
    std::printf(
        "sizeof(cms::util::StaticString<64>)=%zu\n",
        sizeof(cms::util::StaticString<64>));
    return cms::test::finish();
}
