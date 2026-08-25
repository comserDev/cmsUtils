#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/format.h>
#include <cms/parse.h>

#include "test.h"

namespace {

void checkUnsigned(
    cms::StringView input,
    unsigned int base,
    cms::Status status,
    std::uint64_t value,
    std::size_t consumed) {
    const cms::ParseResult<std::uint64_t> result =
        cms::parse::unsignedInteger(input, base);
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.value == value);
    CMS_TEST_CHECK(result.consumed == consumed);
}

void checkSigned(
    cms::StringView input,
    unsigned int base,
    cms::Status status,
    std::int64_t value,
    std::size_t consumed) {
    const cms::ParseResult<std::int64_t> result =
        cms::parse::signedInteger(input, base);
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.value == value);
    CMS_TEST_CHECK(result.consumed == consumed);
}

void checkUnsignedRoundTrip(std::uint64_t value, unsigned int base) {
    char storage[32] = "";
    std::size_t size = 0;
    cms::StringBuffer output(storage, sizeof(storage), size);
    const cms::WriteResult formatted =
        cms::format::unsignedInteger(value, output, base, false);
    CMS_TEST_REQUIRE(formatted.status == cms::Status::ok);
    const cms::ParseResult<std::uint64_t> parsed =
        cms::parse::unsignedInteger(output.view(), base);
    CMS_TEST_CHECK(parsed.status == cms::Status::ok);
    CMS_TEST_CHECK(parsed.value == value);
    CMS_TEST_CHECK(parsed.consumed == output.size());
}

void checkSignedRoundTrip(std::int64_t value, unsigned int base) {
    char storage[32] = "";
    std::size_t size = 0;
    cms::StringBuffer output(storage, sizeof(storage), size);
    const cms::WriteResult formatted =
        cms::format::signedInteger(value, output, base, true);
    CMS_TEST_REQUIRE(formatted.status == cms::Status::ok);
    const cms::ParseResult<std::int64_t> parsed =
        cms::parse::signedInteger(output.view(), base);
    CMS_TEST_CHECK(parsed.status == cms::Status::ok);
    CMS_TEST_CHECK(parsed.value == value);
    CMS_TEST_CHECK(parsed.consumed == output.size());
}

} // namespace

int main() {
    checkUnsigned("0", 10, cms::Status::ok, 0, 1);
    checkUnsigned("42", 10, cms::Status::ok, 42, 2);
    checkUnsigned("123abc", 10, cms::Status::ok, 123, 3);
    checkSigned("0", 10, cms::Status::ok, 0, 1);
    checkSigned("+42", 10, cms::Status::ok, 42, 3);
    checkSigned("-42", 10, cms::Status::ok, -42, 3);
    checkSigned("-0", 10, cms::Status::ok, 0, 2);
    checkSigned("+0", 10, cms::Status::ok, 0, 2);
    checkUnsigned("123Z", 10, cms::Status::ok, 123, 3);
    checkSigned("-123Z", 10, cms::Status::ok, -123, 4);

    checkUnsigned(cms::StringView(), 10, cms::Status::invalid_argument, 0, 0);
    checkUnsigned(
        cms::StringView(nullptr, 8),
        10,
        cms::Status::invalid_argument,
        0,
        0);
    checkUnsigned("a", 10, cms::Status::invalid_argument, 0, 0);
    checkSigned("a", 10, cms::Status::invalid_argument, 0, 0);
    checkSigned("+", 10, cms::Status::invalid_argument, 0, 0);
    checkSigned("-", 10, cms::Status::invalid_argument, 0, 0);
    checkUnsigned("+1", 10, cms::Status::invalid_argument, 0, 0);
    checkUnsigned("-1", 10, cms::Status::invalid_argument, 0, 0);
    checkUnsigned(" 123", 10, cms::Status::invalid_argument, 0, 0);
    checkSigned("\t42", 10, cms::Status::invalid_argument, 0, 0);
    checkUnsigned("1", 2, cms::Status::invalid_argument, 0, 0);
    checkSigned("1", 36, cms::Status::invalid_argument, 0, 0);

    checkUnsigned(
        "18446744073709551615",
        10,
        cms::Status::ok,
        (std::numeric_limits<std::uint64_t>::max)(),
        20);
    checkUnsigned(
        "18446744073709551616",
        10,
        cms::Status::out_of_range,
        0,
        19);
    checkSigned(
        "9223372036854775807",
        10,
        cms::Status::ok,
        (std::numeric_limits<std::int64_t>::max)(),
        19);
    checkSigned(
        "9223372036854775808",
        10,
        cms::Status::out_of_range,
        0,
        18);
    checkSigned(
        "-9223372036854775808",
        10,
        cms::Status::ok,
        (std::numeric_limits<std::int64_t>::min)(),
        20);
    checkSigned(
        "-9223372036854775809",
        10,
        cms::Status::out_of_range,
        0,
        19);

    checkUnsigned("0", 16, cms::Status::ok, 0, 1);
    checkUnsigned("f", 16, cms::Status::ok, 15, 1);
    checkUnsigned("F", 16, cms::Status::ok, 15, 1);
    checkUnsigned("ff", 16, cms::Status::ok, 255, 2);
    checkUnsigned("FF", 16, cms::Status::ok, 255, 2);
    checkUnsigned("0xff", 16, cms::Status::ok, 255, 4);
    checkUnsigned("0XFF", 16, cms::Status::ok, 255, 4);
    checkSigned("-ff", 16, cms::Status::ok, -255, 3);
    checkSigned("+ff", 16, cms::Status::ok, 255, 3);
    checkSigned("-0xff", 16, cms::Status::ok, -255, 5);
    checkSigned("+0XFF", 16, cms::Status::ok, 255, 5);
    checkUnsigned("0xffZ", 16, cms::Status::ok, 255, 4);
    checkSigned("-0xffZ", 16, cms::Status::ok, -255, 5);
    checkUnsigned("0x10", 10, cms::Status::ok, 0, 1);

    checkUnsigned("0x", 16, cms::Status::invalid_argument, 0, 0);
    checkUnsigned("0X", 16, cms::Status::invalid_argument, 0, 0);
    checkUnsigned("0xg", 16, cms::Status::invalid_argument, 0, 0);
    checkSigned("-0x", 16, cms::Status::invalid_argument, 0, 0);
    checkSigned("+0X", 16, cms::Status::invalid_argument, 0, 0);

    checkUnsigned(
        "FFFFFFFFFFFFFFFF",
        16,
        cms::Status::ok,
        (std::numeric_limits<std::uint64_t>::max)(),
        16);
    checkUnsigned(
        "10000000000000000",
        16,
        cms::Status::out_of_range,
        0,
        16);
    checkUnsigned(
        "0x10000000000000000",
        16,
        cms::Status::out_of_range,
        0,
        18);
    checkSigned(
        "7FFFFFFFFFFFFFFF",
        16,
        cms::Status::ok,
        (std::numeric_limits<std::int64_t>::max)(),
        16);
    checkSigned(
        "8000000000000000",
        16,
        cms::Status::out_of_range,
        0,
        15);
    checkSigned(
        "0x8000000000000000",
        16,
        cms::Status::out_of_range,
        0,
        17);
    checkSigned(
        "-8000000000000000",
        16,
        cms::Status::ok,
        (std::numeric_limits<std::int64_t>::min)(),
        17);
    checkSigned(
        "-0x8000000000000000",
        16,
        cms::Status::ok,
        (std::numeric_limits<std::int64_t>::min)(),
        19);
    checkSigned(
        "-8000000000000001",
        16,
        cms::Status::out_of_range,
        0,
        16);
    checkSigned(
        "-0x8000000000000001",
        16,
        cms::Status::out_of_range,
        0,
        18);

    const char embeddedNul[] = {'1', '2', '\0', '3'};
    checkUnsigned(
        cms::StringView(embeddedNul, sizeof(embeddedNul)),
        10,
        cms::Status::ok,
        12,
        2);
    const char highByte[] = {static_cast<char>(0xFF), '1'};
    checkUnsigned(
        cms::StringView(highByte, sizeof(highByte)),
        10,
        cms::Status::invalid_argument,
        0,
        0);

    checkUnsignedRoundTrip(0, 10);
    checkUnsignedRoundTrip(1, 10);
    checkUnsignedRoundTrip(
        (std::numeric_limits<std::uint64_t>::max)(),
        10);
    checkUnsignedRoundTrip(0, 16);
    checkUnsignedRoundTrip(
        (std::numeric_limits<std::uint64_t>::max)(),
        16);
    checkSignedRoundTrip(0, 10);
    checkSignedRoundTrip(1, 10);
    checkSignedRoundTrip(
        (std::numeric_limits<std::int64_t>::max)(),
        10);
    checkSignedRoundTrip(
        (std::numeric_limits<std::int64_t>::min)(),
        10);
    checkSignedRoundTrip(-255, 16);
    checkSignedRoundTrip(
        (std::numeric_limits<std::int64_t>::max)(),
        16);
    checkSignedRoundTrip(
        (std::numeric_limits<std::int64_t>::min)(),
        16);

    std::printf("cms::parse integer coverage complete\n");
    return cms::test::finish();
}
