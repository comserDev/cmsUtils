#include <cstddef>
#include <cstdint>
#include <limits>

#include <cms/util/log/formatter.h>
#include <cms/util/log/record.h>
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

void checkFormatted(
    cms::util::log::Level level,
    cms::util::log::Timestamp timestamp,
    cms::util::StringView message,
    cms::util::StringView expected) {
    cms::util::StaticString<256> output;
    const cms::util::WriteResult result = cms::util::log::format(
        {level, timestamp, message},
        output.buffer());
    checkResult(
        result,
        cms::util::Status::ok,
        expected.size(),
        expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    CMS_TEST_CHECK(cms::util::log::maxFormattedRecordOverhead == 35);

    checkFormatted(
        cms::util::log::Level::trace, 0, cms::util::StringView("x"),
        cms::util::StringView("[0] [TRACE] x\n"));
    checkFormatted(
        cms::util::log::Level::debug, 1, cms::util::StringView("x"),
        cms::util::StringView("[1] [DEBUG] x\n"));
    checkFormatted(
        cms::util::log::Level::info, 2, cms::util::StringView("x"),
        cms::util::StringView("[2] [INFO] x\n"));
    checkFormatted(
        cms::util::log::Level::warning, 3, cms::util::StringView("x"),
        cms::util::StringView("[3] [WARNING] x\n"));
    checkFormatted(
        cms::util::log::Level::error, 4, cms::util::StringView("x"),
        cms::util::StringView("[4] [ERROR] x\n"));
    checkFormatted(
        cms::util::log::Level::critical, 5, cms::util::StringView("x"),
        cms::util::StringView("[5] [CRITICAL] x\n"));
    checkFormatted(
        static_cast<cms::util::log::Level>(0xFF),
        6,
        cms::util::StringView("x"),
        cms::util::StringView("[6] [UNKNOWN] x\n"));

    checkBytes(cms::util::log::levelName(cms::util::log::Level::trace), "TRACE");
    checkBytes(cms::util::log::levelName(cms::util::log::Level::debug), "DEBUG");
    checkBytes(cms::util::log::levelName(cms::util::log::Level::info), "INFO");
    checkBytes(cms::util::log::levelName(cms::util::log::Level::warning), "WARNING");
    checkBytes(cms::util::log::levelName(cms::util::log::Level::error), "ERROR");
    checkBytes(cms::util::log::levelName(cms::util::log::Level::critical), "CRITICAL");
    checkBytes(
        cms::util::log::levelName(static_cast<cms::util::log::Level>(0xFF)),
        "UNKNOWN");

    checkFormatted(
        cms::util::log::Level::critical,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::util::StringView("fatal"),
        cms::util::StringView("[18446744073709551615] [CRITICAL] fatal\n"));
    checkFormatted(
        cms::util::log::Level::info,
        7,
        cms::util::StringView(),
        cms::util::StringView("[7] [INFO] \n"));
    checkFormatted(
        cms::util::log::Level::warning,
        1250,
        cms::util::StringView("sensor"),
        cms::util::StringView("[1250] [WARNING] sensor\n"));

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
        cms::util::log::Level::info,
        8,
        cms::util::StringView(utf8Message, sizeof(utf8Message)),
        cms::util::StringView(utf8Expected, sizeof(utf8Expected)));

    const char embeddedMessage[] = {'A', '\0', 'B'};
    const char embeddedExpected[] = {
        '[', '9', ']', ' ', '[', 'I', 'N', 'F', 'O', ']', ' ',
        'A', '\0', 'B', '\n'};
    checkFormatted(
        cms::util::log::Level::info,
        9,
        cms::util::StringView(embeddedMessage, sizeof(embeddedMessage)),
        cms::util::StringView(embeddedExpected, sizeof(embeddedExpected)));

    char exactStorage[14] = {};
    std::size_t exactSize = 0;
    cms::util::StringBuffer exactOutput(exactStorage, sizeof(exactStorage), exactSize);
    const cms::util::WriteResult exact = cms::util::log::format(
        {cms::util::log::Level::info, 0, cms::util::StringView("x")},
        exactOutput);
    checkResult(exact, cms::util::Status::ok, 13, 13);
    CMS_TEST_CHECK(exactSize == 13);
    CMS_TEST_CHECK(exactStorage[13] == '\0');
    checkBytes(exactOutput.view(), "[0] [INFO] x\n");

    char shortStorage[13] = {'o', 'l', 'd', '\0'};
    std::size_t shortSize = 3;
    cms::util::StringBuffer shortOutput(shortStorage, sizeof(shortStorage), shortSize);
    const cms::util::WriteResult shortResult = cms::util::log::format(
        {cms::util::log::Level::info, 0, cms::util::StringView("x")},
        shortOutput);
    checkResult(shortResult, cms::util::Status::no_space, 0, 13);
    CMS_TEST_CHECK(shortSize == 3);
    checkBytes(shortOutput.view(), "old");
    CMS_TEST_CHECK(shortStorage[3] == '\0');

    const cms::util::WriteResult invalid = cms::util::log::format(
        {cms::util::log::Level::info, 0, cms::util::StringView("x")},
        cms::util::StringBuffer());
    checkResult(invalid, cms::util::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    cms::util::StringBuffer overflowOutput(
        overflowStorage,
        sizeof(overflowStorage),
        overflowSize);
    const cms::util::WriteResult overflow = cms::util::log::format(
        {
            cms::util::log::Level::info,
            0,
            cms::util::StringView(
                "x",
                (std::numeric_limits<std::size_t>::max)())
        },
        overflowOutput);
    checkResult(overflow, cms::util::Status::out_of_range, 0, 0);
    CMS_TEST_CHECK(overflowSize == 3);
    checkBytes(overflowOutput.view(), "old");

    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); ++index) {
        maximumMessage[index] = 'm';
    }
    cms::util::log::StaticRecord<64> maximumRecord;
    CMS_TEST_REQUIRE(maximumRecord.assign(
        cms::util::log::Level::critical,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::util::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::util::Status::ok);

    cms::util::StaticString<64 + cms::util::log::maxFormattedRecordOverhead> maximumLine;
    const cms::util::WriteResult maximumResult = cms::util::log::format(
        maximumRecord.view(),
        maximumLine.buffer());
    checkResult(maximumResult, cms::util::Status::ok, 98, 98);
    CMS_TEST_CHECK(maximumLine.size() == 98);
    CMS_TEST_CHECK(maximumLine.cStr()[98] == '\0');
    CMS_TEST_CHECK(maximumLine.view()[97] == '\n');

    cms::util::StaticString<98> maximumShort;
    CMS_TEST_REQUIRE(maximumShort.assign("old").status == cms::util::Status::ok);
    const cms::util::WriteResult maximumShortResult = cms::util::log::format(
        maximumRecord.view(),
        maximumShort.buffer());
    checkResult(maximumShortResult, cms::util::Status::no_space, 0, 98);
    checkBytes(maximumShort.view(), "old");

    cms::util::StaticString<64> aliased;
    CMS_TEST_REQUIRE(aliased.assign("hello").status == cms::util::Status::ok);
    const cms::util::StringView aliasedMessage = aliased.view();
    const cms::util::WriteResult aliasedResult = cms::util::log::format(
        {cms::util::log::Level::info, 0, aliasedMessage},
        aliased.buffer());
    checkResult(aliasedResult, cms::util::Status::ok, 17, 17);
    checkBytes(aliased.view(), "[0] [INFO] hello\n");
    CMS_TEST_CHECK(aliased.cStr()[aliased.size()] == '\0');

    return cms::test::finish();
}
