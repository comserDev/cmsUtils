#include <cstddef>
#include <cstdint>
#include <limits>

#include <cms/log/formatter.h>
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
    const cms::WriteResult result = cms::log::format(
        {level, timestamp, message},
        output.buffer());
    checkResult(
        result,
        cms::Status::ok,
        expected.size(),
        expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    CMS_TEST_CHECK(cms::log::maxFormattedRecordOverhead == 35);

    checkFormatted(
        cms::log::Level::trace, 0, cms::StringView("x"),
        cms::StringView("[0] [TRACE] x\n"));
    checkFormatted(
        cms::log::Level::debug, 1, cms::StringView("x"),
        cms::StringView("[1] [DEBUG] x\n"));
    checkFormatted(
        cms::log::Level::info, 2, cms::StringView("x"),
        cms::StringView("[2] [INFO] x\n"));
    checkFormatted(
        cms::log::Level::warning, 3, cms::StringView("x"),
        cms::StringView("[3] [WARNING] x\n"));
    checkFormatted(
        cms::log::Level::error, 4, cms::StringView("x"),
        cms::StringView("[4] [ERROR] x\n"));
    checkFormatted(
        cms::log::Level::critical, 5, cms::StringView("x"),
        cms::StringView("[5] [CRITICAL] x\n"));
    checkFormatted(
        static_cast<cms::log::Level>(0xFF),
        6,
        cms::StringView("x"),
        cms::StringView("[6] [UNKNOWN] x\n"));

    checkBytes(cms::log::levelName(cms::log::Level::trace), "TRACE");
    checkBytes(cms::log::levelName(cms::log::Level::debug), "DEBUG");
    checkBytes(cms::log::levelName(cms::log::Level::info), "INFO");
    checkBytes(cms::log::levelName(cms::log::Level::warning), "WARNING");
    checkBytes(cms::log::levelName(cms::log::Level::error), "ERROR");
    checkBytes(cms::log::levelName(cms::log::Level::critical), "CRITICAL");
    checkBytes(
        cms::log::levelName(static_cast<cms::log::Level>(0xFF)),
        "UNKNOWN");

    checkFormatted(
        cms::log::Level::critical,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::StringView("fatal"),
        cms::StringView("[18446744073709551615] [CRITICAL] fatal\n"));
    checkFormatted(
        cms::log::Level::info,
        7,
        cms::StringView(),
        cms::StringView("[7] [INFO] \n"));
    checkFormatted(
        cms::log::Level::warning,
        1250,
        cms::StringView("sensor"),
        cms::StringView("[1250] [WARNING] sensor\n"));

    const char utf8Message[] = {
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC)};
    const char utf8Expected[] = {
        '[', '8', ']', ' ', '[', 'I', 'N', 'F', 'O', ']', ' ',
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC),
        '\n'};
    checkFormatted(
        cms::log::Level::info,
        8,
        cms::StringView(utf8Message, sizeof(utf8Message)),
        cms::StringView(utf8Expected, sizeof(utf8Expected)));

    const char embeddedMessage[] = {'A', '\0', 'B'};
    const char embeddedExpected[] = {
        '[', '9', ']', ' ', '[', 'I', 'N', 'F', 'O', ']', ' ',
        'A', '\0', 'B', '\n'};
    checkFormatted(
        cms::log::Level::info,
        9,
        cms::StringView(embeddedMessage, sizeof(embeddedMessage)),
        cms::StringView(embeddedExpected, sizeof(embeddedExpected)));

    char exactStorage[14] = {};
    std::size_t exactSize = 0;
    cms::StringBuffer exactOutput(exactStorage, sizeof(exactStorage), exactSize);
    const cms::WriteResult exact = cms::log::format(
        {cms::log::Level::info, 0, cms::StringView("x")},
        exactOutput);
    checkResult(exact, cms::Status::ok, 13, 13);
    CMS_TEST_CHECK(exactSize == 13);
    CMS_TEST_CHECK(exactStorage[13] == '\0');
    checkBytes(exactOutput.view(), "[0] [INFO] x\n");

    char shortStorage[13] = {'o', 'l', 'd', '\0'};
    std::size_t shortSize = 3;
    cms::StringBuffer shortOutput(shortStorage, sizeof(shortStorage), shortSize);
    const cms::WriteResult shortResult = cms::log::format(
        {cms::log::Level::info, 0, cms::StringView("x")},
        shortOutput);
    checkResult(shortResult, cms::Status::no_space, 0, 13);
    CMS_TEST_CHECK(shortSize == 3);
    checkBytes(shortOutput.view(), "old");
    CMS_TEST_CHECK(shortStorage[3] == '\0');

    const cms::WriteResult invalid = cms::log::format(
        {cms::log::Level::info, 0, cms::StringView("x")},
        cms::StringBuffer());
    checkResult(invalid, cms::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    cms::StringBuffer overflowOutput(
        overflowStorage,
        sizeof(overflowStorage),
        overflowSize);
    const cms::WriteResult overflow = cms::log::format(
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

    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); ++index) {
        maximumMessage[index] = 'm';
    }
    cms::log::StaticRecord<64> maximumRecord;
    CMS_TEST_REQUIRE(maximumRecord.assign(
        cms::log::Level::critical,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::Status::ok);

    cms::StaticString<64 + cms::log::maxFormattedRecordOverhead> maximumLine;
    const cms::WriteResult maximumResult = cms::log::format(
        maximumRecord.view(),
        maximumLine.buffer());
    checkResult(maximumResult, cms::Status::ok, 98, 98);
    CMS_TEST_CHECK(maximumLine.size() == 98);
    CMS_TEST_CHECK(maximumLine.cStr()[98] == '\0');
    CMS_TEST_CHECK(maximumLine.view()[97] == '\n');

    cms::StaticString<98> maximumShort;
    CMS_TEST_REQUIRE(maximumShort.assign("old").status == cms::Status::ok);
    const cms::WriteResult maximumShortResult = cms::log::format(
        maximumRecord.view(),
        maximumShort.buffer());
    checkResult(maximumShortResult, cms::Status::no_space, 0, 98);
    checkBytes(maximumShort.view(), "old");

    cms::StaticString<64> aliased;
    CMS_TEST_REQUIRE(aliased.assign("hello").status == cms::Status::ok);
    const cms::StringView aliasedMessage = aliased.view();
    const cms::WriteResult aliasedResult = cms::log::format(
        {cms::log::Level::info, 0, aliasedMessage},
        aliased.buffer());
    checkResult(aliasedResult, cms::Status::ok, 17, 17);
    checkBytes(aliased.view(), "[0] [INFO] hello\n");
    CMS_TEST_CHECK(aliased.cStr()[aliased.size()] == '\0');

    return cms::test::finish();
}
