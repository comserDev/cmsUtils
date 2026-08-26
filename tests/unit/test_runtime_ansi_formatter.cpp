#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/formatter.h>
#include <cms/util/log/runtime_ansi_formatter.h>
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

void checkBytes(cms::util::StringView actual, cms::util::StringView expected) {
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
    cms::util::log::RuntimeAnsiFormatter& formatter,
    bool useColor,
    const cms::util::log::Record& record) {
    formatter.setUseColor(useColor);

    cms::util::StaticString<256> expected;
    const cms::util::WriteResult expectedResult = useColor
        ? cms::util::log::formatAnsi(record, expected.buffer())
        : cms::util::log::format(record, expected.buffer());
    CMS_TEST_REQUIRE(expectedResult.status == cms::util::Status::ok);

    cms::util::StaticString<256> actual;
    const cms::util::WriteResult actualResult = formatter.format(
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

void checkAliased(bool useColor, cms::util::StringView expected) {
    cms::util::log::RuntimeAnsiFormatter formatter;
    formatter.setUseColor(useColor);

    cms::util::StaticString<64> output;
    CMS_TEST_REQUIRE(output.assign("hello").status == cms::util::Status::ok);
    const cms::util::StringView message = output.view();
    const cms::util::WriteResult result = formatter.format(
        {cms::util::log::Level::info, 0, message},
        output.buffer());
    checkResult(result, cms::util::Status::ok, expected.size(), expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    cms::util::log::RuntimeAnsiFormatter formatter;
    CMS_TEST_CHECK(formatter.useColor());
    CMS_TEST_CHECK(
        cms::util::log::RuntimeAnsiFormatter::maxOverhead
        == cms::util::log::maxAnsiFormattedRecordOverhead);
    CMS_TEST_CHECK(cms::util::log::RuntimeAnsiFormatter::maxOverhead == 43);

    formatter.setUseColor(false);
    CMS_TEST_CHECK(!formatter.useColor());
    formatter.setUseColor(true);
    CMS_TEST_CHECK(formatter.useColor());
    formatter.setUseColor(false);
    CMS_TEST_CHECK(!formatter.useColor());
    formatter.setUseColor(true);
    CMS_TEST_CHECK(formatter.useColor());

    const cms::util::log::Level levels[] = {
        cms::util::log::Level::trace,
        cms::util::log::Level::debug,
        cms::util::log::Level::info,
        cms::util::log::Level::warning,
        cms::util::log::Level::error,
        cms::util::log::Level::critical,
        static_cast<cms::util::log::Level>(0xFF)};
    for (std::size_t index = 0;
         index < sizeof(levels) / sizeof(levels[0]);
         ++index) {
        checkMatchesReference(
            formatter,
            false,
            {levels[index], static_cast<cms::util::log::Timestamp>(index), "x"});
        checkMatchesReference(
            formatter,
            true,
            {levels[index], static_cast<cms::util::log::Timestamp>(index), "x"});
    }

    checkMatchesReference(
        formatter,
        false,
        {cms::util::log::Level::info, 10, cms::util::StringView()});
    checkMatchesReference(
        formatter,
        true,
        {cms::util::log::Level::info, 10, cms::util::StringView()});

    const char embeddedMessage[] = {'A', '\0', 'B'};
    checkMatchesReference(
        formatter,
        false,
        {cms::util::log::Level::error, 11,
         cms::util::StringView(embeddedMessage, sizeof(embeddedMessage))});
    checkMatchesReference(
        formatter,
        true,
        {cms::util::log::Level::error, 11,
         cms::util::StringView(embeddedMessage, sizeof(embeddedMessage))});

    const char utf8Message[] = {
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC)};
    checkMatchesReference(
        formatter,
        false,
        {cms::util::log::Level::warning, 12,
         cms::util::StringView(utf8Message, sizeof(utf8Message))});
    checkMatchesReference(
        formatter,
        true,
        {cms::util::log::Level::warning, 12,
         cms::util::StringView(utf8Message, sizeof(utf8Message))});

    formatter.setUseColor(false);
    cms::util::StaticString<64> plainOutput;
    const cms::util::WriteResult plain = formatter.format(
        {cms::util::log::Level::info, 10, "hello"},
        plainOutput.buffer());
    checkResult(plain, cms::util::Status::ok, 18, 18);
    checkBytes(plainOutput.view(), "[10] [INFO] hello\n");

    formatter.setUseColor(true);
    cms::util::StaticString<64> ansiOutput;
    const cms::util::WriteResult ansi = formatter.format(
        {cms::util::log::Level::info, 10, "hello"},
        ansiOutput.buffer());
    checkResult(ansi, cms::util::Status::ok, 27, 27);
    checkBytes(
        ansiOutput.view(),
        "[10] \033[32m[INFO]\033[0m hello\n");

    constexpr char plainExact[] = "[0] [INFO] x\n";
    char plainExactStorage[sizeof(plainExact)] = {};
    std::size_t plainExactSize = 0;
    formatter.setUseColor(false);
    const cms::util::WriteResult plainExactResult = formatter.format(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer(
            plainExactStorage,
            sizeof(plainExactStorage),
            plainExactSize));
    checkResult(
        plainExactResult,
        cms::util::Status::ok,
        sizeof(plainExact) - 1,
        sizeof(plainExact) - 1);
    checkBytes(
        cms::util::StringView(plainExactStorage, plainExactSize),
        plainExact);

    char plainShortStorage[sizeof(plainExact) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t plainShortSize = 3;
    const cms::util::WriteResult plainShort = formatter.format(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer(
            plainShortStorage,
            sizeof(plainShortStorage),
            plainShortSize));
    checkResult(
        plainShort,
        cms::util::Status::no_space,
        0,
        sizeof(plainExact) - 1);
    CMS_TEST_CHECK(plainShortSize == 3);
    checkBytes(cms::util::StringView(plainShortStorage, plainShortSize), "old");
    CMS_TEST_CHECK(plainShortStorage[3] == '\0');

    constexpr char ansiExact[] = "[0] \033[32m[INFO]\033[0m x\n";
    char ansiExactStorage[sizeof(ansiExact)] = {};
    std::size_t ansiExactSize = 0;
    formatter.setUseColor(true);
    const cms::util::WriteResult ansiExactResult = formatter.format(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer(
            ansiExactStorage,
            sizeof(ansiExactStorage),
            ansiExactSize));
    checkResult(
        ansiExactResult,
        cms::util::Status::ok,
        sizeof(ansiExact) - 1,
        sizeof(ansiExact) - 1);
    checkBytes(
        cms::util::StringView(ansiExactStorage, ansiExactSize),
        ansiExact);

    char ansiShortStorage[sizeof(ansiExact) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t ansiShortSize = 3;
    const cms::util::WriteResult ansiShort = formatter.format(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer(
            ansiShortStorage,
            sizeof(ansiShortStorage),
            ansiShortSize));
    checkResult(
        ansiShort,
        cms::util::Status::no_space,
        0,
        sizeof(ansiExact) - 1);
    CMS_TEST_CHECK(ansiShortSize == 3);
    checkBytes(cms::util::StringView(ansiShortStorage, ansiShortSize), "old");
    CMS_TEST_CHECK(ansiShortStorage[3] == '\0');

    const cms::util::WriteResult invalid = formatter.format(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer());
    checkResult(invalid, cms::util::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    const cms::util::WriteResult overflow = formatter.format(
        {
            cms::util::log::Level::info,
            0,
            cms::util::StringView(
                "x",
                (std::numeric_limits<std::size_t>::max)())
        },
        cms::util::StringBuffer(
            overflowStorage,
            sizeof(overflowStorage),
            overflowSize));
    checkResult(overflow, cms::util::Status::out_of_range, 0, 0);
    CMS_TEST_CHECK(overflowSize == 3);
    checkBytes(cms::util::StringView(overflowStorage, overflowSize), "old");

    checkAliased(false, "[0] [INFO] hello\n");
    checkAliased(true, "[0] \033[32m[INFO]\033[0m hello\n");

    std::printf(
        "sizeof(cms::util::log::PlainFormatter)=%zu\n",
        sizeof(cms::util::log::PlainFormatter));
    std::printf(
        "sizeof(cms::util::log::AnsiFormatter)=%zu\n",
        sizeof(cms::util::log::AnsiFormatter));
    std::printf(
        "sizeof(cms::util::log::RuntimeAnsiFormatter)=%zu\n",
        sizeof(cms::util::log::RuntimeAnsiFormatter));

    return cms::test::finish();
}
