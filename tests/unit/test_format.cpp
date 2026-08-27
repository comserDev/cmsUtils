#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/util/format.h>
#include <cms/util/static_string.h>

#include "test.h"

namespace {

void checkResult(
    const cms::util::WriteResult& result,
    cms::util::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

void checkBuffer(
    cms::util::StringBuffer output,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(output.valid());
    CMS_TEST_CHECK(output.size() == expectedSize);
    CMS_TEST_REQUIRE(output.data() != nullptr);
    CMS_TEST_CHECK(output.data()[output.size()] == '\0');
    for (std::size_t index = 0; index < expectedSize; ++index) {
        CMS_TEST_CHECK(output.data()[index] == expected[index]);
    }
}

void checkUnsigned(
    std::uint64_t value,
    unsigned int base,
    bool uppercase,
    const char* expected,
    std::size_t expectedSize) {
    char storage[32] = "old";
    std::size_t size = 3;
    cms::util::StringBuffer output(storage, sizeof(storage), size);
    checkResult(
        cms::util::format::unsignedInteger(value, output, base, uppercase),
        cms::util::Status::ok,
        expectedSize,
        expectedSize);
    checkBuffer(output, expected, expectedSize);
}

void checkSigned(
    std::int64_t value,
    unsigned int base,
    bool uppercase,
    const char* expected,
    std::size_t expectedSize) {
    char storage[32] = "old";
    std::size_t size = 3;
    cms::util::StringBuffer output(storage, sizeof(storage), size);
    checkResult(
        cms::util::format::signedInteger(value, output, base, uppercase),
        cms::util::Status::ok,
        expectedSize,
        expectedSize);
    checkBuffer(output, expected, expectedSize);
}

void checkFloating(
    double value,
    unsigned int decimalPlaces,
    const char* expected,
    std::size_t expectedSize) {
    char storage[40] = "old";
    std::size_t size = 3;
    cms::util::StringBuffer output(storage, sizeof(storage), size);
    checkResult(
        cms::util::format::floatingPoint(value, output, decimalPlaces),
        cms::util::Status::ok,
        expectedSize,
        expectedSize);
    checkBuffer(output, expected, expectedSize);
}

} // namespace

int main() {
    checkUnsigned(0, 10, false, "0", 1);
    checkUnsigned(1, 10, false, "1", 1);
    checkUnsigned(9, 10, false, "9", 1);
    checkUnsigned(10, 10, false, "10", 2);
    checkUnsigned(42, 10, true, "42", 2);
    checkUnsigned(
        (std::numeric_limits<std::uint32_t>::max)(),
        10,
        false,
        "4294967295",
        10);
    checkUnsigned(
        (std::numeric_limits<std::uint64_t>::max)(),
        10,
        false,
        "18446744073709551615",
        20);

    checkSigned(0, 10, false, "0", 1);
    checkSigned(1, 10, false, "1", 1);
    checkSigned(-1, 10, false, "-1", 2);
    checkSigned(42, 10, false, "42", 2);
    checkSigned(-42, 10, false, "-42", 3);
    checkSigned(
        (std::numeric_limits<std::int32_t>::min)(),
        10,
        false,
        "-2147483648",
        11);
    checkSigned(
        (std::numeric_limits<std::int32_t>::max)(),
        10,
        false,
        "2147483647",
        10);
    checkSigned(
        (std::numeric_limits<std::int64_t>::min)(),
        10,
        false,
        "-9223372036854775808",
        20);
    checkSigned(
        (std::numeric_limits<std::int64_t>::max)(),
        10,
        false,
        "9223372036854775807",
        19);

    checkUnsigned(0, 16, false, "0", 1);
    checkUnsigned(0xAU, 16, false, "a", 1);
    checkUnsigned(0xAU, 16, true, "A", 1);
    checkUnsigned(0xFU, 16, false, "f", 1);
    checkUnsigned(0x10U, 16, false, "10", 2);
    checkUnsigned(0xFFU, 16, false, "ff", 2);
    checkUnsigned(0xFFU, 16, true, "FF", 2);
    checkUnsigned(
        (std::numeric_limits<std::uint64_t>::max)(),
        16,
        false,
        "ffffffffffffffff",
        16);

    checkFloating(0.0, 0, "0", 1);
    checkFloating(0.0, 2, "0.00", 4);
    checkFloating(12.5, 2, "12.50", 5);
    checkFloating(-12.5, 2, "-12.50", 6);
    checkFloating(1.25, 1, "1.3", 3);
    checkFloating(-1.25, 1, "-1.3", 4);
    checkFloating(std::nextafter(0.05, 0.0), 1, "0.0", 3);
    checkFloating(std::nextafter(0.05, 1.0), 1, "0.1", 3);
    checkFloating(std::nextafter(-0.05, 0.0), 1, "-0.0", 4);
    checkFloating(std::nextafter(-0.05, -1.0), 1, "-0.1", 4);
    checkFloating(std::nextafter(0.00000005, 0.0), 7, "0.0000000", 9);
    checkFloating(std::nextafter(0.00000005, 1.0), 7, "0.0000001", 9);
    checkFloating(1.999, 2, "2.00", 4);
    checkFloating(-0.0, 2, "-0.00", 5);
    checkFloating(1.0, 9, "1.000000000", 11);
    checkFloating(9007199254740991.0, 0, "9007199254740991", 16);
    checkFloating(
        18446744073709549568.0,
        0,
        "18446744073709549568",
        20);
    checkUnsigned(
        (std::numeric_limits<std::uint64_t>::max)(),
        16,
        true,
        "FFFFFFFFFFFFFFFF",
        16);
    checkSigned(-255, 16, false, "-ff", 3);
    checkSigned(-255, 16, true, "-FF", 3);
    checkSigned(
        (std::numeric_limits<std::int64_t>::min)(),
        16,
        false,
        "-8000000000000000",
        17);
    checkSigned(
        (std::numeric_limits<std::int64_t>::max)(),
        16,
        true,
        "7FFFFFFFFFFFFFFF",
        16);

    char emptyStorage[8] = "";
    std::size_t emptySize = 0;
    cms::util::StringBuffer emptyOutput(
        emptyStorage,
        sizeof(emptyStorage),
        emptySize);
    checkResult(
        cms::util::format::unsignedInteger(42, emptyOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(emptyOutput, "42", 2);

    char exactStorage[3] = "x";
    std::size_t exactSize = 1;
    cms::util::StringBuffer exactOutput(exactStorage, sizeof(exactStorage), exactSize);
    checkResult(
        cms::util::format::unsignedInteger(42, exactOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(exactOutput, "42", 2);

    char shortStorage[2] = "x";
    std::size_t shortSize = 1;
    cms::util::StringBuffer shortOutput(shortStorage, sizeof(shortStorage), shortSize);
    checkResult(
        cms::util::format::unsignedInteger(42, shortOutput),
        cms::util::Status::no_space,
        0,
        2);
    checkBuffer(shortOutput, "x", 1);

    char signedMaximumStorage[21] = "";
    std::size_t signedMaximumSize = 0;
    cms::util::StringBuffer signedMaximumOutput(
        signedMaximumStorage,
        sizeof(signedMaximumStorage),
        signedMaximumSize);
    CMS_TEST_CHECK(signedMaximumOutput.maxSize() == 20);
    checkResult(
        cms::util::format::signedInteger(
            (std::numeric_limits<std::int64_t>::min)(),
            signedMaximumOutput),
        cms::util::Status::ok,
        20,
        20);
    checkBuffer(signedMaximumOutput, "-9223372036854775808", 20);

    char signedShortStorage[20] = "old";
    const char signedShortExpected[20] = "old";
    std::size_t signedShortSize = 3;
    cms::util::StringBuffer signedShortOutput(
        signedShortStorage,
        sizeof(signedShortStorage),
        signedShortSize);
    CMS_TEST_CHECK(signedShortOutput.maxSize() == 19);
    checkResult(
        cms::util::format::signedInteger(
            (std::numeric_limits<std::int64_t>::min)(),
            signedShortOutput),
        cms::util::Status::no_space,
        0,
        20);
    checkBuffer(signedShortOutput, "old", 3);
    for (std::size_t index = 0; index < sizeof(signedShortStorage); ++index) {
        CMS_TEST_CHECK(signedShortStorage[index] == signedShortExpected[index]);
    }

    char unsignedMaximumStorage[21] = "";
    std::size_t unsignedMaximumSize = 0;
    cms::util::StringBuffer unsignedMaximumOutput(
        unsignedMaximumStorage,
        sizeof(unsignedMaximumStorage),
        unsignedMaximumSize);
    CMS_TEST_CHECK(unsignedMaximumOutput.maxSize() == 20);
    checkResult(
        cms::util::format::unsignedInteger(
            (std::numeric_limits<std::uint64_t>::max)(),
            unsignedMaximumOutput),
        cms::util::Status::ok,
        20,
        20);
    checkBuffer(unsignedMaximumOutput, "18446744073709551615", 20);

    char unsignedShortStorage[20] = "old";
    const char unsignedShortExpected[20] = "old";
    std::size_t unsignedShortSize = 3;
    cms::util::StringBuffer unsignedShortOutput(
        unsignedShortStorage,
        sizeof(unsignedShortStorage),
        unsignedShortSize);
    CMS_TEST_CHECK(unsignedShortOutput.maxSize() == 19);
    checkResult(
        cms::util::format::unsignedInteger(
            (std::numeric_limits<std::uint64_t>::max)(),
            unsignedShortOutput),
        cms::util::Status::no_space,
        0,
        20);
    checkBuffer(unsignedShortOutput, "old", 3);
    for (std::size_t index = 0; index < sizeof(unsignedShortStorage); ++index) {
        CMS_TEST_CHECK(
            unsignedShortStorage[index] == unsignedShortExpected[index]);
    }

    char appendStorage[6] = "ab";
    std::size_t appendSize = 2;
    cms::util::StringBuffer appendOutput(
        appendStorage,
        sizeof(appendStorage),
        appendSize);
    checkResult(
        cms::util::format::appendSignedInteger(-42, appendOutput),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(appendOutput, "ab-42", 5);
    checkResult(
        cms::util::format::appendUnsignedInteger(10, appendOutput),
        cms::util::Status::no_space,
        0,
        2);
    checkBuffer(appendOutput, "ab-42", 5);

    char unsignedAppendStorage[6] = "id=";
    std::size_t unsignedAppendSize = 3;
    cms::util::StringBuffer unsignedAppendOutput(
        unsignedAppendStorage,
        sizeof(unsignedAppendStorage),
        unsignedAppendSize);
    checkResult(
        cms::util::format::appendUnsignedInteger(42, unsignedAppendOutput),
        cms::util::Status::ok,
        2,
        2);
    checkBuffer(unsignedAppendOutput, "id=42", 5);

    cms::util::StaticString<1> tiny;
    checkResult(
        cms::util::format::unsignedInteger(0, tiny.buffer()),
        cms::util::Status::no_space,
        0,
        1);
    CMS_TEST_CHECK(tiny.empty());
    CMS_TEST_CHECK(tiny.cStr()[0] == '\0');

    cms::util::StringBuffer defaultOutput;
    checkResult(
        cms::util::format::unsignedInteger(1, defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::signedInteger(-1, defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::appendUnsignedInteger(1, defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::appendSignedInteger(-1, defaultOutput),
        cms::util::Status::invalid_argument,
        0,
        0);

    char damagedStorage[8] = "old";
    std::size_t damagedSize = 3;
    cms::util::StringBuffer damagedOutput(
        damagedStorage,
        sizeof(damagedStorage),
        damagedSize);
    damagedSize = sizeof(damagedStorage);
    checkResult(
        cms::util::format::unsignedInteger(1, damagedOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    CMS_TEST_CHECK(damagedSize == sizeof(damagedStorage));
    CMS_TEST_CHECK(damagedStorage[0] == 'o');
    CMS_TEST_CHECK(damagedStorage[1] == 'l');
    CMS_TEST_CHECK(damagedStorage[2] == 'd');
    CMS_TEST_CHECK(damagedStorage[3] == '\0');

    char invalidBaseStorage[8] = "old";
    std::size_t invalidBaseSize = 3;
    cms::util::StringBuffer invalidBaseOutput(
        invalidBaseStorage,
        sizeof(invalidBaseStorage),
        invalidBaseSize);
    checkResult(
        cms::util::format::unsignedInteger(1, invalidBaseOutput, 2),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::appendSignedInteger(-1, invalidBaseOutput, 36),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkBuffer(invalidBaseOutput, "old", 3);

    char invalidFloatStorage[16] = "old";
    std::size_t invalidFloatSize = 3;
    cms::util::StringBuffer invalidFloatOutput(
        invalidFloatStorage,
        sizeof(invalidFloatStorage),
        invalidFloatSize);
    checkResult(
        cms::util::format::floatingPoint(1.0, invalidFloatOutput, 10),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::floatingPoint(
            (std::numeric_limits<double>::quiet_NaN)(),
            invalidFloatOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::floatingPoint(
            (std::numeric_limits<double>::infinity)(),
            invalidFloatOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::floatingPoint(
            -(std::numeric_limits<double>::infinity)(),
            invalidFloatOutput),
        cms::util::Status::invalid_argument,
        0,
        0);
    checkResult(
        cms::util::format::floatingPoint(
            18446744073709551616.0,
            invalidFloatOutput,
            0),
        cms::util::Status::out_of_range,
        0,
        0);
    checkBuffer(invalidFloatOutput, "old", 3);

    char exactFloatStorage[5] = "x";
    std::size_t exactFloatSize = 1;
    cms::util::StringBuffer exactFloatOutput(
        exactFloatStorage,
        sizeof(exactFloatStorage),
        exactFloatSize);
    checkResult(
        cms::util::format::floatingPoint(1.25, exactFloatOutput, 2),
        cms::util::Status::ok,
        4,
        4);
    checkBuffer(exactFloatOutput, "1.25", 4);

    char shortFloatStorage[4] = "old";
    std::size_t shortFloatSize = 3;
    cms::util::StringBuffer shortFloatOutput(
        shortFloatStorage,
        sizeof(shortFloatStorage),
        shortFloatSize);
    checkResult(
        cms::util::format::floatingPoint(1.25, shortFloatOutput, 2),
        cms::util::Status::no_space,
        0,
        4);
    checkBuffer(shortFloatOutput, "old", 3);

    char appendFloatStorage[10] = "v=";
    std::size_t appendFloatSize = 2;
    cms::util::StringBuffer appendFloatOutput(
        appendFloatStorage,
        sizeof(appendFloatStorage),
        appendFloatSize);
    checkResult(
        cms::util::format::appendFloatingPoint(1.25, appendFloatOutput, 1),
        cms::util::Status::ok,
        3,
        3);
    checkBuffer(appendFloatOutput, "v=1.3", 5);
    checkResult(
        cms::util::format::appendFloatingPoint(12.50, appendFloatOutput, 2),
        cms::util::Status::no_space,
        0,
        5);
    checkBuffer(appendFloatOutput, "v=1.3", 5);

    std::printf("cms::util::format coverage complete\n");
    return cms::test::finish();
}
