#include <cstddef>
#include <cstdint>
#include <limits>

#include <cms/util/log/styled_ansi_formatter.h>
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
    cms::util::StaticString<512> output;
    const cms::util::WriteResult result = cms::util::log::formatStyledAnsi(
        {level, timestamp, message},
        output.buffer());
    checkResult(result, cms::util::Status::ok, expected.size(), expected.size());
    checkBytes(output.view(), expected);
    CMS_TEST_CHECK(output.cStr()[output.size()] == '\0');
}

} // namespace

int main() {
    CMS_TEST_CHECK(cms::util::log::maxStyledMessageExpansionFactor == 4);
    CMS_TEST_CHECK(cms::util::log::styledFormattedStorageAdjustment == 40);

    checkFormatted(
        cms::util::log::Level::info,
        0,
        "plain",
        "[0] \033[32m[INFO]\033[0m plain\n");
    checkFormatted(
        cms::util::log::Level::info,
        1,
        "[]",
        "[1] \033[32m[INFO]\033[0m []\n");
    checkFormatted(
        cms::util::log::Level::debug,
        2,
        "[A]",
        "[2] \033[36m[DEBUG]\033[0m \033[32m[A]\033[0m\n");
    checkFormatted(
        cms::util::log::Level::warning,
        3,
        "[Network]",
        "[3] \033[33m[WARNING]\033[0m \033[94m[Network]\033[0m\n");
    checkFormatted(
        cms::util::log::Level::warning,
        4,
        "[network]",
        "[4] \033[33m[WARNING]\033[0m \033[94m[network]\033[0m\n");
    checkFormatted(
        cms::util::log::Level::error,
        5,
        "[NET][NET]",
        "[5] \033[31m[ERROR]\033[0m \033[95m[NET]\033[0m\033[95m[NET]\033[0m\n");
    checkFormatted(
        cms::util::log::Level::info,
        6,
        "[LONG_TAG_123]",
        "[6] \033[32m[INFO]\033[0m \033[32m[LONG_TAG_123]\033[0m\n");

    checkFormatted(
        cms::util::log::Level::info,
        7,
        "ERROR CRITICAL FATAL FAIL",
        "[7] \033[32m[INFO]\033[0m "
        "\033[1;91mERROR\033[0m \033[1;91mCRITICAL\033[0m "
        "\033[1;91mFATAL\033[0m \033[1;91mFAIL\033[0m\n");
    checkFormatted(
        cms::util::log::Level::info,
        8,
        "errorCriticalfatalfail",
        "[8] \033[32m[INFO]\033[0m "
        "\033[1;91merror\033[0m\033[1;91mCritical\033[0m"
        "\033[1;91mfatal\033[0m\033[1;91mfail\033[0m\n");
    checkFormatted(
        cms::util::log::Level::info,
        9,
        "prefailure suffix",
        "[9] \033[32m[INFO]\033[0m pre\033[1;91mfail\033[0mure suffix\n");
    checkFormatted(
        cms::util::log::Level::info,
        10,
        "ordinary",
        "[10] \033[32m[INFO]\033[0m ordinary\n");

    checkFormatted(
        cms::util::log::Level::info,
        11,
        "[NET] ERROR failed",
        "[11] \033[32m[INFO]\033[0m \033[95m[NET]\033[0m "
        "\033[1;91mERROR\033[0m \033[1;91mfail\033[0med\n");
    checkFormatted(
        cms::util::log::Level::critical,
        12,
        "[ERROR] FAIL",
        "[12] \033[0m[CRITICAL] \033[92m[ERROR]\033[0m "
        "\033[1;91mFAIL\033[0m\n");

    const char embeddedMessage[] = {
        'A', '\0', '[', 'N', 'E', 'T', ']', 'f', 'a', 'i', 'l'};
    const char embeddedExpected[] = {
        '[', '1', '3', ']', ' ', '\033', '[', '3', '2', 'm',
        '[', 'I', 'N', 'F', 'O', ']', '\033', '[', '0', 'm', ' ',
        'A', '\0', '\033', '[', '9', '5', 'm', '[', 'N', 'E', 'T', ']',
        '\033', '[', '0', 'm', '\033', '[', '1', ';', '9', '1', 'm',
        'f', 'a', 'i', 'l', '\033', '[', '0', 'm', '\n'};
    checkFormatted(
        cms::util::log::Level::info,
        13,
        cms::util::StringView(embeddedMessage, sizeof(embeddedMessage)),
        cms::util::StringView(embeddedExpected, sizeof(embeddedExpected)));

    const char utf8Message[] = {
        static_cast<char>(0xE2), static_cast<char>(0x82),
        static_cast<char>(0xAC), ' ', '[', 'A', ']', ' ',
        static_cast<char>(0xEA), static_cast<char>(0xB0),
        static_cast<char>(0x80), ' ', 'F', 'A', 'I', 'L'};
    const char utf8Expected[] = {
        '[', '1', '4', ']', ' ', '\033', '[', '3', '2', 'm',
        '[', 'I', 'N', 'F', 'O', ']', '\033', '[', '0', 'm', ' ',
        static_cast<char>(0xE2), static_cast<char>(0x82),
        static_cast<char>(0xAC), ' ', '\033', '[', '3', '2', 'm',
        '[', 'A', ']', '\033', '[', '0', 'm', ' ',
        static_cast<char>(0xEA), static_cast<char>(0xB0),
        static_cast<char>(0x80), ' ', '\033', '[', '1', ';', '9', '1', 'm',
        'F', 'A', 'I', 'L', '\033', '[', '0', 'm', '\n'};
    checkFormatted(
        cms::util::log::Level::info,
        14,
        cms::util::StringView(utf8Message, sizeof(utf8Message)),
        cms::util::StringView(utf8Expected, sizeof(utf8Expected)));

    cms::util::StaticString<128> aliased;
    CMS_TEST_REQUIRE(aliased.assign("[A] ERROR").status == cms::util::Status::ok);
    const cms::util::WriteResult aliasedResult = cms::util::log::formatStyledAnsi(
        {cms::util::log::Level::debug, 0, aliased.view()},
        aliased.buffer());
    const cms::util::StringView aliasedExpected =
        "[0] \033[36m[DEBUG]\033[0m \033[32m[A]\033[0m "
        "\033[1;91mERROR\033[0m\n";
    checkResult(
        aliasedResult,
        cms::util::Status::ok,
        aliasedExpected.size(),
        aliasedExpected.size());
    checkBytes(aliased.view(), aliasedExpected);

    constexpr char exactExpected[] =
        "[0] \033[32m[INFO]\033[0m \033[32m[A]\033[0m\n";
    char exactStorage[sizeof(exactExpected)] = {};
    std::size_t exactSize = 0;
    cms::util::StringBuffer exactOutput(exactStorage, sizeof(exactStorage), exactSize);
    const cms::util::WriteResult exact = cms::util::log::formatStyledAnsi(
        {cms::util::log::Level::info, 0, "[A]"},
        exactOutput);
    checkResult(
        exact,
        cms::util::Status::ok,
        sizeof(exactExpected) - 1,
        sizeof(exactExpected) - 1);
    checkBytes(exactOutput.view(), exactExpected);

    char shortStorage[sizeof(exactExpected) - 1] = {'o', 'l', 'd', '\0'};
    std::size_t shortSize = 3;
    cms::util::StringBuffer shortOutput(shortStorage, sizeof(shortStorage), shortSize);
    const cms::util::WriteResult shortResult = cms::util::log::formatStyledAnsi(
        {cms::util::log::Level::info, 0, "[A]"},
        shortOutput);
    checkResult(
        shortResult,
        cms::util::Status::no_space,
        0,
        sizeof(exactExpected) - 1);
    CMS_TEST_CHECK(shortSize == 3);
    checkBytes(shortOutput.view(), "old");

    const cms::util::WriteResult invalid = cms::util::log::formatStyledAnsi(
        {cms::util::log::Level::info, 0, "x"},
        cms::util::StringBuffer());
    checkResult(invalid, cms::util::Status::invalid_argument, 0, 0);

    char overflowStorage[8] = {'o', 'l', 'd', '\0'};
    std::size_t overflowSize = 3;
    cms::util::StringBuffer overflowOutput(
        overflowStorage,
        sizeof(overflowStorage),
        overflowSize);
    const cms::util::WriteResult overflow = cms::util::log::formatStyledAnsi(
        {cms::util::log::Level::info, 0,
            cms::util::StringView(
                "x",
                (std::numeric_limits<std::size_t>::max)())},
        overflowOutput);
    checkResult(overflow, cms::util::Status::out_of_range, 0, 0);
    CMS_TEST_CHECK(overflowSize == 3);
    checkBytes(overflowOutput.view(), "old");

    char maximumMessage[63] = {};
    for (std::size_t index = 0; index < sizeof(maximumMessage); index += 3) {
        maximumMessage[index] = '[';
        maximumMessage[index + 1] = 'x';
        maximumMessage[index + 2] = ']';
    }
    cms::util::log::StaticRecord<64> maximumRecord;
    CMS_TEST_REQUIRE(maximumRecord.assign(
        cms::util::log::Level::warning,
        (std::numeric_limits<std::uint64_t>::max)(),
        cms::util::StringView(maximumMessage, sizeof(maximumMessage))).status
        == cms::util::Status::ok);
    cms::util::StaticString<
        64 * cms::util::log::maxStyledMessageExpansionFactor
            + cms::util::log::styledFormattedStorageAdjustment>
        maximumLine;
    const cms::util::WriteResult maximumResult = cms::util::log::formatStyledAnsi(
        maximumRecord.view(),
        maximumLine.buffer());
    checkResult(maximumResult, cms::util::Status::ok, 295, 295);
    CMS_TEST_CHECK(maximumLine.size() == 295);
    CMS_TEST_CHECK(maximumLine.view()[294] == '\n');
    CMS_TEST_CHECK(maximumLine.cStr()[295] == '\0');

    cms::util::log::RuntimeStyledAnsiFormatter runtime;
    CMS_TEST_CHECK(runtime.useColor());
    runtime.setUseColor(false);
    cms::util::StaticString<64> runtimePlain;
    const cms::util::WriteResult runtimePlainResult = runtime.format(
        {cms::util::log::Level::info, 0, "[A] FAIL"},
        runtimePlain.buffer());
    CMS_TEST_REQUIRE(runtimePlainResult.status == cms::util::Status::ok);
    checkBytes(runtimePlain.view(), "[0] [INFO] [A] FAIL\n");
    runtime.setUseColor(true);
    CMS_TEST_CHECK(runtime.useColor());

    return cms::test::finish();
}
