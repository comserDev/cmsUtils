#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/log/ansi_formatter.h>
#include <cms/log/formatter.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/static_string.h>

#include "test.h"

namespace {

void checkResult(
    const cms::WriteResult& result,
    cms::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

void checkBytes(cms::StringView actual, cms::StringView expected) {
    CMS_TEST_REQUIRE(actual.size() == expected.size());
    if (!expected.empty()) {
        CMS_TEST_REQUIRE(actual.data() != nullptr);
        CMS_TEST_REQUIRE(expected.data() != nullptr);
    }
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

void checkMatchesReference(
    cms::log::RuntimeAnsiFormatter& formatter,
    bool useColor,
    const cms::log::Record& record) {
    formatter.setUseColor(useColor);

    cms::StaticString<256> expected;
    const cms::WriteResult expectedResult = useColor
        ? cms::log::formatAnsi(record, expected.buffer())
        : cms::log::format(record, expected.buffer());
    CMS_TEST_REQUIRE(expectedResult.status == cms::Status::ok);

    cms::StaticString<256> actual;
    const cms::WriteResult actualResult = formatter.format(
        record,
        actual.buffer());
    checkResult(
        actualResult,
        expectedResult.status,
        expectedResult.written,
        expectedResult.required);
    checkBytes(actual.view(), expected.view());
    CMS_TEST_CHECK(actual.cStr()[actual.size()] == '\0');
}

void checkAliased(bool useColor, cms::StringView expected) {
    cms::log::RuntimeAnsiFormatter formatter;
    formatter.setUseColor(useColor);

    cms::StaticString<64> output;
    CMS_TEST_REQUIRE(output.assign("hello").status == cms::Status::ok);
    const cms::StringView message = output.view();
    const cms::WriteResult result = formatter.format(
        {cms::log::Level::info, 0, message},
        output.buffer());
    checkResult(result, cms::Status::ok, expected.size(), expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    cms::log::RuntimeAnsiFormatter formatter;
    CMS_TEST_CHECK(formatter.useColor());
    CMS_TEST_CHECK(
        cms::log::RuntimeAnsiFormatter::maxOverhead
        == cms::log::maxAnsiFormattedRecordOverhead);
    CMS_TEST_CHECK(cms::log::RuntimeAnsiFormatter::maxOverhead == 43);

    formatter.setUseColor(false);
    CMS_TEST_CHECK(!formatter.useColor());
    formatter.setUseColor(true);
    CMS_TEST_CHECK(formatter.useColor());
    formatter.setUseColor(false);
    CMS_TEST_CHECK(!formatter.useColor());
    formatter.setUseColor(true);
    CMS_TEST_CHECK(formatter.useColor());

    const cms::log::Level levels[] = {
        cms::log::Level::trace,
        cms::log::Level::debug,
        cms::log::Level::info,
        cms::log::Level::warning,
        cms::log::Level::error,
        cms::log::Level::critical,
        static_cast<cms::log::Level>(0xFF)};
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        checkMatchesReference(
            formatter,
            false,
            {levels[index], static_cast<cms::log::Timestamp>(index), "x"});
        checkMatchesReference(
            formatter,
            true,
            {levels[index], static_cast<cms::log::Timestamp>(index), "x"});
    }

    checkMatchesReference(
        formatter,
        false,
        {cms::log::Level::info, 10, cms::StringView()});
    checkMatchesReference(
        formatter,
        true,
        {cms::log::Level::info, 10, cms::StringView()});

    const char embeddedMessage[] = {'A', '\0', 'B'};
    checkMatchesReference(
        formatter,
        false,
        {cms::log::Level::error, 11,
         cms::StringView(embeddedMessage, sizeof(embeddedMessage))});
    checkMatchesReference(
        formatter,
        true,
        {cms::log::Level::error, 11,
         cms::StringView(embeddedMessage, sizeof(embeddedMessage))});

    const char utf8Message[] = {
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC)};
    checkMatchesReference(
        formatter,
        false,
        {cms::log::Level::warning, 12,
         cms::StringView(utf8Message, sizeof(utf8Message))});
    checkMatchesReference(
        formatter,
        true,
        {cms::log::Level::warning, 12,
         cms::StringView(utf8Message, sizeof(utf8Message))});

    formatter.setUseColor(false);
    cms::StaticString<64> plainOutput;
    const cms::WriteResult plain = formatter.format(
        {cms::log::Level::info, 10, "hello"},
        plainOutput.buffer());
    checkResult(plain, cms::Status::ok, 18, 18);
    checkBytes(plainOutput.view(), "[10] [INFO] hello\n");

    formatter.setUseColor(true);
    cms::StaticString<64> ansiOutput;
    const cms::WriteResult ansi = formatter.format(
        {cms::log::Level::info, 10, "hello"},
        ansiOutput.buffer());
    checkResult(ansi, cms::Status::ok, 27, 27);
    checkBytes(
        ansiOutput.view(),
        "[10] \033[32m[INFO]\033[0m hello\n");

    constexpr char plainExact[] = "[0] [INFO] x\n";
    char plainExactStorage[sizeof(plainExact)] = {};
    std::size_t plainExactSize = 0;
    formatter.setUseColor(false);
    const cms::WriteResult plainExactResult = formatter.format(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer(
            plainExactStorage,
            sizeof(plainExactStorage),
            plainExactSize));
    checkResult(
        plainExactResult,
        cms::Status::ok,
        sizeof(plainExact) - 1,
        sizeof(plainExact) - 1);
    checkBytes(
        cms::StringView(plainExactStorage, plainExactSize),
        plainExact);

    char plainShortStorage[sizeof(plainExact) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t plainShortSize = 3;
    const cms::WriteResult plainShort = formatter.format(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer(
            plainShortStorage,
            sizeof(plainShortStorage),
            plainShortSize));
    checkResult(
        plainShort,
        cms::Status::no_space,
        0,
        sizeof(plainExact) - 1);
    CMS_TEST_CHECK(plainShortSize == 3);
    checkBytes(cms::StringView(plainShortStorage, plainShortSize), "old");
    CMS_TEST_CHECK(plainShortStorage[3] == '\0');

    constexpr char ansiExact[] = "[0] \033[32m[INFO]\033[0m x\n";
    char ansiExactStorage[sizeof(ansiExact)] = {};
    std::size_t ansiExactSize = 0;
    formatter.setUseColor(true);
    const cms::WriteResult ansiExactResult = formatter.format(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer(
            ansiExactStorage,
            sizeof(ansiExactStorage),
            ansiExactSize));
    checkResult(
        ansiExactResult,
        cms::Status::ok,
        sizeof(ansiExact) - 1,
        sizeof(ansiExact) - 1);
    checkBytes(
        cms::StringView(ansiExactStorage, ansiExactSize),
        ansiExact);

    char ansiShortStorage[sizeof(ansiExact) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t ansiShortSize = 3;
    const cms::WriteResult ansiShort = formatter.format(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer(
            ansiShortStorage,
            sizeof(ansiShortStorage),
            ansiShortSize));
    checkResult(
        ansiShort,
        cms::Status::no_space,
        0,
        sizeof(ansiExact) - 1);
    CMS_TEST_CHECK(ansiShortSize == 3);
    checkBytes(cms::StringView(ansiShortStorage, ansiShortSize), "old");
    CMS_TEST_CHECK(ansiShortStorage[3] == '\0');

    const cms::WriteResult invalid = formatter.format(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer());
    checkResult(invalid, cms::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    const cms::WriteResult overflow = formatter.format(
        {
            cms::log::Level::info,
            0,
            cms::StringView(
                "x",
                (std::numeric_limits<std::size_t>::max)())
        },
        cms::StringBuffer(
            overflowStorage,
            sizeof(overflowStorage),
            overflowSize));
    checkResult(overflow, cms::Status::out_of_range, 0, 0);
    CMS_TEST_CHECK(overflowSize == 3);
    checkBytes(cms::StringView(overflowStorage, overflowSize), "old");

    checkAliased(false, "[0] [INFO] hello\n");
    checkAliased(true, "[0] \033[32m[INFO]\033[0m hello\n");

    std::printf(
        "sizeof(cms::log::PlainFormatter)=%zu\n",
        sizeof(cms::log::PlainFormatter));
    std::printf(
        "sizeof(cms::log::AnsiFormatter)=%zu\n",
        sizeof(cms::log::AnsiFormatter));
    std::printf(
        "sizeof(cms::log::RuntimeAnsiFormatter)=%zu\n",
        sizeof(cms::log::RuntimeAnsiFormatter));

    return cms::test::finish();
}
