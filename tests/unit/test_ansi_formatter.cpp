#include <cstddef>
#include <cstdint>
#include <limits>

#include <cms/log/ansi_formatter.h>
#include <cms/log/record.h>
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

void checkFormatted(
    cms::log::Level level,
    cms::log::Timestamp timestamp,
    cms::StringView message,
    cms::StringView expected) {
    cms::StaticString<256> output;
    const cms::WriteResult result = cms::log::formatAnsi(
        {level, timestamp, message},
        output.buffer());
    checkResult(result, cms::Status::ok, expected.size(), expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    CMS_TEST_CHECK(cms::log::maxAnsiFormattedRecordOverhead == 43);
    CMS_TEST_CHECK(
        cms::log::AnsiFormatter::maxOverhead
        == cms::log::maxAnsiFormattedRecordOverhead);

    checkFormatted(
        cms::log::Level::trace,
        0,
        "x",
        "[0] \033[0m[TRACE] x\n");
    checkFormatted(
        cms::log::Level::debug,
        1,
        "x",
        "[1] \033[36m[DEBUG]\033[0m x\n");
    checkFormatted(
        cms::log::Level::info,
        2,
        "x",
        "[2] \033[32m[INFO]\033[0m x\n");
    checkFormatted(
        cms::log::Level::warning,
        3,
        "x",
        "[3] \033[33m[WARNING]\033[0m x\n");
    checkFormatted(
        cms::log::Level::error,
        4,
        "x",
        "[4] \033[31m[ERROR]\033[0m x\n");
    checkFormatted(
        cms::log::Level::critical,
        5,
        "x",
        "[5] \033[0m[CRITICAL] x\n");
    checkFormatted(
        static_cast<cms::log::Level>(0xFF),
        6,
        "x",
        "[6] \033[0m[UNKNOWN] x\n");

    checkFormatted(
        cms::log::Level::warning,
        (std::numeric_limits<std::uint64_t>::max)(),
        "fatal",
        "[18446744073709551615] \033[33m[WARNING]\033[0m fatal\n");
    checkFormatted(
        cms::log::Level::info,
        7,
        cms::StringView(),
        "[7] \033[32m[INFO]\033[0m \n");
    checkFormatted(
        cms::log::Level::debug,
        8,
        "normal",
        "[8] \033[36m[DEBUG]\033[0m normal\n");

    const char utf8Message[] = {
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC)};
    const char utf8Expected[] = {
        '[', '9', ']', ' ', '\033', '[', '3', '2', 'm',
        '[', 'I', 'N', 'F', 'O', ']', '\033', '[', '0', 'm', ' ',
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC),
        '\n'};
    checkFormatted(
        cms::log::Level::info,
        9,
        cms::StringView(utf8Message, sizeof(utf8Message)),
        cms::StringView(utf8Expected, sizeof(utf8Expected)));

    const char embeddedMessage[] = {'A', '\0', 'B'};
    const char embeddedExpected[] = {
        '[', '1', '0', ']', ' ', '\033', '[', '3', '1', 'm',
        '[', 'E', 'R', 'R', 'O', 'R', ']', '\033', '[', '0', 'm', ' ',
        'A', '\0', 'B', '\n'};
    checkFormatted(
        cms::log::Level::error,
        10,
        cms::StringView(embeddedMessage, sizeof(embeddedMessage)),
        cms::StringView(embeddedExpected, sizeof(embeddedExpected)));

    constexpr char exactExpected[] = "[0] \033[36m[DEBUG]\033[0m x\n";
    char exactStorage[sizeof(exactExpected)] = {};
    std::size_t exactSize = 0;
    cms::StringBuffer exactOutput(exactStorage, sizeof(exactStorage), exactSize);
    const cms::WriteResult exact = cms::log::formatAnsi(
        {cms::log::Level::debug, 0, "x"},
        exactOutput);
    const std::size_t exactPayload = sizeof(exactExpected) - 1;
    checkResult(exact, cms::Status::ok, exactPayload, exactPayload);
    checkBytes(exactOutput.view(), exactExpected);
    CMS_TEST_CHECK(exactStorage[exactSize] == '\0');

    char shortStorage[sizeof(exactExpected) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t shortSize = 3;
    cms::StringBuffer shortOutput(shortStorage, sizeof(shortStorage), shortSize);
    const cms::WriteResult shortResult = cms::log::formatAnsi(
        {cms::log::Level::debug, 0, "x"},
        shortOutput);
    checkResult(
        shortResult,
        cms::Status::no_space,
        0,
        exactPayload);
    CMS_TEST_CHECK(shortSize == 3);
    checkBytes(shortOutput.view(), "old");
    CMS_TEST_CHECK(shortStorage[3] == '\0');

    const cms::WriteResult invalid = cms::log::formatAnsi(
        {cms::log::Level::info, 0, "x"},
        cms::StringBuffer());
    checkResult(invalid, cms::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    cms::StringBuffer overflowOutput(
        overflowStorage,
        sizeof(overflowStorage),
        overflowSize);
    const cms::WriteResult overflow = cms::log::formatAnsi(
        {
            cms::log::Level::info,
            0,
            cms::StringView(
                "x",
                (std::numeric_limits<std::size_t>::max)())
        },
        overflowOutput);
    checkResult(overflow, cms::Status::out_of_range, 0, 0);
    CMS_TEST_CHECK(overflowSize == 3);
    checkBytes(overflowOutput.view(), "old");

    cms::StaticString<64> aliased;
    CMS_TEST_REQUIRE(aliased.assign("hello").status == cms::Status::ok);
    const cms::StringView aliasedMessage = aliased.view();
    const cms::StringView aliasedExpected =
        "[0] \033[32m[INFO]\033[0m hello\n";
    const cms::WriteResult aliasedResult = cms::log::formatAnsi(
        {cms::log::Level::info, 0, aliasedMessage},
        aliased.buffer());
    checkResult(
        aliasedResult,
        cms::Status::ok,
        aliasedExpected.size(),
        aliasedExpected.size());
    checkBytes(aliased.view(), aliasedExpected);
    CMS_TEST_CHECK(aliased.cStr()[aliased.size()] == '\0');

    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); ++index) {
        maximumMessage[index] = 'm';
    }
    cms::log::StaticRecord<64> maximumRecord;
    CMS_TEST_REQUIRE(maximumRecord.assign(
        cms::log::Level::warning,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::Status::ok);

    cms::StaticString<64 + cms::log::maxAnsiFormattedRecordOverhead>
        maximumLine;
    const cms::WriteResult maximumResult = cms::log::formatAnsi(
        maximumRecord.view(),
        maximumLine.buffer());
    checkResult(maximumResult, cms::Status::ok, 106, 106);
    CMS_TEST_CHECK(maximumLine.size() == 106);
    CMS_TEST_CHECK(maximumLine.view()[105] == '\n');
    CMS_TEST_CHECK(maximumLine.cStr()[106] == '\0');

    cms::StaticString<106> maximumShort;
    CMS_TEST_REQUIRE(maximumShort.assign("old").status == cms::Status::ok);
    const cms::WriteResult maximumShortResult = cms::log::formatAnsi(
        maximumRecord.view(),
        maximumShort.buffer());
    checkResult(maximumShortResult, cms::Status::no_space, 0, 106);
    checkBytes(maximumShort.view(), "old");

    return cms::test::finish();
}
