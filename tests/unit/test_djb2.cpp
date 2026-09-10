#include <cstddef>
#include <cstdint>

#include <cms/util/hash/djb2.h>

#include "test.h"

namespace {

using cms::util::ByteView;
using cms::util::StringView;
using cms::util::hash::Djb2;

void checkByteVector(ByteView input, std::uint32_t expected) {
    CMS_TEST_CHECK(cms::util::hash::djb2(input) == expected);
    Djb2 incremental;
    incremental.update(input);
    CMS_TEST_CHECK(incremental.value() == expected);
}

void checkStringVector(StringView input, std::uint32_t expected) {
    CMS_TEST_CHECK(cms::util::hash::djb2(input) == expected);
    Djb2 incremental;
    incremental.update(input);
    CMS_TEST_CHECK(incremental.value() == expected);
}

void testVectors() {
    checkStringVector(StringView(), 0x00001505U);
    checkStringVector(StringView("a"), 0x0002B606U);
    checkStringVector(StringView("abc"), 0x0B885C8BU);
    checkStringVector(StringView("hello"), 0x0F923099U);
    checkStringVector(StringView("NAMU001"), 0x153E3EA7U);

    const std::uint8_t bytes[] = {0x00, 0xFF, 0x80, 0x01};
    checkByteVector(ByteView(bytes), 0x7C615CC5U);
}

void testChunkedUpdatesAndReset() {
    const std::uint8_t first[] = {'a', 'b'};
    const std::uint8_t second[] = {'c'};
    Djb2 hash;
    CMS_TEST_CHECK(hash.value() == cms::util::hash::Djb2InitialValue);
    hash.update(ByteView(first));
    hash.update(ByteView(second));
    CMS_TEST_CHECK(hash.value() == 0x0B885C8BU);

    hash.reset();
    CMS_TEST_CHECK(hash.value() == cms::util::hash::Djb2InitialValue);
    hash.update(StringView("hello"));
    CMS_TEST_CHECK(hash.value() == 0x0F923099U);
}

void testByteExactAndCaseSensitive() {
    const std::uint8_t embedded[] = {0x00, 0xFF, 0x80, 0x01};
    const std::uint8_t sameBytes[] = {0x00, 0xFF, 0x80, 0x01};
    CMS_TEST_CHECK(cms::util::hash::djb2(ByteView(embedded)) ==
                  cms::util::hash::djb2(ByteView(sameBytes)));
    CMS_TEST_CHECK(cms::util::hash::djb2(StringView("abc")) !=
                  cms::util::hash::djb2(StringView("ABC")));

    const char stringBytes[] = {
        static_cast<char>(0x00), static_cast<char>(0xFF),
        static_cast<char>(0x80), static_cast<char>(0x01)};
    CMS_TEST_CHECK(cms::util::hash::djb2(
                       StringView(stringBytes, sizeof(stringBytes))) ==
                  0x7C615CC5U);
}

} // namespace

int main() {
    testVectors();
    testChunkedUpdatesAndReset();
    testByteExactAndCaseSensitive();
    return cms::test::finish();
}
