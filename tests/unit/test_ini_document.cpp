#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <cms/util/ini/document.h>
#include <cms/util/ini/file.h>

#include "test.h"

namespace {

using cms::util::Status;
using cms::util::ini::Document;
using cms::util::ini::TextMode;

const std::filesystem::path root =
    std::filesystem::temp_directory_path() / "cms_utils_ini_document_test";

void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void cleanup() {
    std::error_code error;
    std::filesystem::remove_all(root, error);
}

Status loadPath(
    const std::filesystem::path& path,
    Document& document,
    TextMode mode = TextMode::automatic) {
    return cms::util::ini::loadFile(path.string().c_str(), document, mode);
}

Status savePath(const std::filesystem::path& path, const Document& document) {
    return cms::util::ini::saveFile(path.string().c_str(), document);
}

void testReadWriteAndMissingValues() {
    Document document;
    CMS_TEST_CHECK(!document["Network"].contains("ssid"));
    CMS_TEST_CHECK(!document["Network"]["ssid"].exists());

    const std::string missing = document["Network"]["ssid"];
    CMS_TEST_CHECK(missing.empty());
    CMS_TEST_CHECK(!document.contains("Network", "ssid"));

    document["Network"]["ssid"] = "MyWifi";
    document["Network"]["port"] = std::string("8080");
    CMS_TEST_CHECK(document["Network"]["ssid"].exists());
    CMS_TEST_CHECK(
        document["Network"]["ssid"].get() == "MyWifi");
    CMS_TEST_CHECK(
        static_cast<std::string>(document["Network"]["port"]) == "8080");
    document.clear();
    CMS_TEST_CHECK(!document.contains("Network", "ssid"));
}

void testGlobalAndProxyLifetime() {
    cleanup();
    std::filesystem::create_directories(root);
    Document document;
    document[""]["version"] = "2";
    CMS_TEST_CHECK(document[""]["version"].get() == "2");
    const auto path = root / "global.ini";
    CMS_TEST_CHECK(cms::util::ini::saveFile(path.string().c_str(), document) ==
                   Status::ok);
    CMS_TEST_CHECK(readFile(path).find("[]") == std::string::npos);

    auto value = document["Stable"]["key"];
    document.setValue("Other", "unrelated", "value");
    value = "updated";
    CMS_TEST_CHECK(document.getValue("Stable", "key") == "updated");
}

void testDuplicateUpdateUsesExistingSemantics() {
    cleanup();
    std::filesystem::create_directories(root);
    const auto path = root / "duplicates.ini";
    writeFile(path,
        "[Dup]\n"
        "value=first\n"
        "[Other]\n"
        "other=1\n"
        "[Dup]\n"
        "value=last\n");

    Document document;
    CMS_TEST_REQUIRE(
        cms::util::ini::loadFile(path.string().c_str(), document) ==
        Status::ok);
    document["Dup"]["value"] = "updated";
    document["Dup"]["new"] = "inserted";
    CMS_TEST_CHECK(
        cms::util::ini::saveFile(path.string().c_str(), document) ==
        Status::ok);

    const std::string saved = readFile(path);
    CMS_TEST_CHECK(saved.find("value=first\n") != std::string::npos);
    CMS_TEST_CHECK(saved.find("value = updated\n") != std::string::npos);
    CMS_TEST_CHECK(
        saved.find("new = inserted\n") != std::string::npos);
}

void testConstReadOnlyProxy() {
    Document document;
    document["Network"]["ssid"] = "MyWifi";
    const Document& readOnly = document;
    const std::string value = readOnly["Network"]["ssid"];
    CMS_TEST_CHECK(value == "MyWifi");
    CMS_TEST_CHECK(readOnly["Network"].contains("ssid"));
    CMS_TEST_CHECK(!readOnly["Network"]["missing"].exists());
}

void testHostAdapterFailureIsTransactional() {
    cleanup();
    std::filesystem::create_directories(root);
    const auto path = root / "transaction.ini";
    writeFile(path, "[State]\nvalue=before\n");

    Document document;
    CMS_TEST_REQUIRE(
        cms::util::ini::loadFile(path.string().c_str(), document) ==
        Status::ok);
    std::filesystem::remove(path);
    CMS_TEST_CHECK(
        cms::util::ini::loadFile(path.string().c_str(), document) ==
        Status::io_error);
    CMS_TEST_CHECK(document.getValue("State", "value") == "before");
    CMS_TEST_CHECK(
        cms::util::ini::loadFile(nullptr, document) ==
        Status::invalid_argument);
    CMS_TEST_CHECK(
        cms::util::ini::saveFile(nullptr, document) ==
        Status::invalid_argument);
}

void testParsingAndPreservation() {
    cleanup();
    std::filesystem::create_directories(root);

    const auto basic = root / "basic.ini";
    const std::string basicSource =
        "; semicolon comment\n"
        "# hash comment\n"
        "\n"
        "version = 2\n"
        "unknown line\n"
        "[System]\n"
        "name=value\n"
        "url = a=b=c\n"
        "empty=\n"
        "[Network]\n"
        "port = 1234\n";
    writeFile(basic, basicSource);
    Document document;
    CMS_TEST_REQUIRE(loadPath(basic, document) == Status::ok);
    CMS_TEST_CHECK(document.getValue("", "version") == "2");
    CMS_TEST_CHECK(document.getValue("System", "name") == "value");
    CMS_TEST_CHECK(document.getValue("System", "url") == "a=b=c");
    CMS_TEST_CHECK(document.getValue("System", "empty", "fallback").empty());
    CMS_TEST_CHECK(document.contains("Network", "port"));
    CMS_TEST_CHECK(!document.contains("network", "port"));
    CMS_TEST_CHECK(savePath(basic, document) == Status::ok);
    CMS_TEST_CHECK(readFile(basic) == basicSource);

    const auto empty = root / "empty.ini";
    writeFile(empty, "");
    Document emptyDocument;
    CMS_TEST_CHECK(loadPath(empty, emptyDocument) == Status::ok);
    CMS_TEST_CHECK(
        emptyDocument.getValue("", "missing", "default") == "default");
    CMS_TEST_CHECK(savePath(empty, emptyDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(empty).empty());

    const auto malformed = root / "malformed.ini";
    const std::string malformedSource =
        "[]\n"
        "[   ]\n"
        "after=global\n"
        "key=value ; comment\n"
        "[Valid]\n"
        "x=1\n";
    writeFile(malformed, malformedSource);
    Document malformedDocument;
    CMS_TEST_REQUIRE(loadPath(malformed, malformedDocument) == Status::ok);
    CMS_TEST_CHECK(malformedDocument.getValue("", "after") == "global");
    CMS_TEST_CHECK(
        malformedDocument.getValue("", "key") == "value ; comment");
    CMS_TEST_CHECK(malformedDocument.getValue("Valid", "x") == "1");
    CMS_TEST_CHECK(savePath(malformed, malformedDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(malformed) == malformedSource);

    const auto lf = root / "lf.ini";
    const std::string lfSource = "; c\n\n[One]\na=1\n";
    writeFile(lf, lfSource);
    Document lfDocument;
    CMS_TEST_REQUIRE(loadPath(lf, lfDocument) == Status::ok);
    CMS_TEST_CHECK(savePath(lf, lfDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(lf) == lfSource);

    const auto crlf = root / "crlf.ini";
    const std::string crlfSource = "# c\r\n[One]\r\na=1\r\n";
    writeFile(crlf, crlfSource);
    Document crlfDocument;
    CMS_TEST_REQUIRE(loadPath(crlf, crlfDocument) == Status::ok);
    CMS_TEST_CHECK(savePath(crlf, crlfDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(crlf) == crlfSource);

    const auto noFinal = root / "no-final.ini";
    writeFile(noFinal, "[One]\r\na=1");
    Document noFinalDocument;
    CMS_TEST_REQUIRE(loadPath(noFinal, noFinalDocument) == Status::ok);
    CMS_TEST_CHECK(noFinalDocument.setValue("One", "b", "2") == Status::ok);
    CMS_TEST_CHECK(savePath(noFinal, noFinalDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(noFinal) == "[One]\r\na=1\r\nb = 2");

    const auto saveAs = root / "save-as.ini";
    CMS_TEST_CHECK(savePath(saveAs, noFinalDocument) == Status::ok);
    CMS_TEST_CHECK(readFile(saveAs) == readFile(noFinal));
}

void testInvalidFileArguments() {
    Document document;
    CMS_TEST_CHECK(
        cms::util::ini::loadFile("", document) == Status::invalid_argument);
    CMS_TEST_CHECK(
        cms::util::ini::saveFile("", document) == Status::invalid_argument);
    CMS_TEST_CHECK(
        cms::util::ini::loadFile(nullptr, document) == Status::invalid_argument);
    CMS_TEST_CHECK(
        cms::util::ini::saveFile(nullptr, document) == Status::invalid_argument);
}

void testTextModesAndUtf8Detection() {
    cleanup();
    std::filesystem::create_directories(root);

    const auto utf8Path = root / "utf8.ini";
    writeFile(
        utf8Path,
        u8"[네트워크]\n이름 = 나무001\n설명 = 자전거 경기 계측 장치\n");
    Document utf8Document;
    CMS_TEST_REQUIRE(loadPath(utf8Path, utf8Document) == Status::ok);
    CMS_TEST_CHECK(utf8Document.getValue(u8"네트워크", u8"이름") == u8"나무001");
    CMS_TEST_CHECK(
        utf8Document.getValue(u8"네트워크", u8"설명") == u8"자전거 경기 계측 장치");

    const auto asciiPath = root / "ascii.ini";
    writeFile(asciiPath, "[Network]\nname = NAMU001\n");
    Document asciiDocument;
    CMS_TEST_REQUIRE(loadPath(asciiPath, asciiDocument) == Status::ok);
    CMS_TEST_CHECK(asciiDocument.getValue("Network", "name") == "NAMU001");

    const auto invalidPath = root / "invalid-fallback.ini";
    const std::string invalidSource =
        "[State]\nvalue=before\nraw=" +
        std::string(1, static_cast<char>(0xFF)) + "\n";
    writeFile(invalidPath, invalidSource);
    Document invalidDocument;
    CMS_TEST_REQUIRE(loadPath(invalidPath, invalidDocument) == Status::ok);
    CMS_TEST_CHECK(invalidDocument.getValue("State", "value") == "before");
    CMS_TEST_CHECK(
        invalidDocument.getValue("State", "raw") ==
        std::string(1, static_cast<char>(0xFF)));

    Document forcedUtf8Document;
    forcedUtf8Document["State"]["value"] = "stable";
    Document forcedUtf8ValidDocument;
    CMS_TEST_REQUIRE(
        loadPath(utf8Path, forcedUtf8ValidDocument, TextMode::utf8) ==
        Status::ok);
    CMS_TEST_CHECK(
        forcedUtf8ValidDocument.getValue(u8"네트워크", u8"이름") == u8"나무001");
    CMS_TEST_CHECK(
        loadPath(invalidPath, forcedUtf8Document, TextMode::utf8) ==
        Status::invalid_utf8);
    CMS_TEST_CHECK(
        forcedUtf8Document.getValue("State", "value") == "stable");

    Document forcedAsciiInvalidDocument;
    CMS_TEST_REQUIRE(
        loadPath(invalidPath, forcedAsciiInvalidDocument, TextMode::ascii) ==
        Status::ok);
    CMS_TEST_CHECK(
        forcedAsciiInvalidDocument.getValue("State", "raw") ==
        std::string(1, static_cast<char>(0xFF)));

    const std::string fullWidthSpace = u8"　";
    const auto unicodeSpacePath = root / "unicode-space.ini";
    writeFile(
        unicodeSpacePath,
        "[" + fullWidthSpace + u8"네트워크" + fullWidthSpace + "]\n" +
        fullWidthSpace + u8"이름" + fullWidthSpace + "=" +
        fullWidthSpace + u8"나무001" + fullWidthSpace + "\n");
    Document unicodeDocument;
    CMS_TEST_REQUIRE(loadPath(unicodeSpacePath, unicodeDocument) == Status::ok);
    CMS_TEST_CHECK(
        unicodeDocument.getValue(u8"네트워크", u8"이름") == u8"나무001");

    Document forcedAsciiDocument;
    CMS_TEST_REQUIRE(
        loadPath(unicodeSpacePath, forcedAsciiDocument, TextMode::ascii) ==
        Status::ok);
    CMS_TEST_CHECK(
        forcedAsciiDocument.getValue(
            fullWidthSpace + u8"네트워크" + fullWidthSpace,
            fullWidthSpace + u8"이름" + fullWidthSpace) ==
        fullWidthSpace + u8"나무001" + fullWidthSpace);
}

void testBomHandling() {
    cleanup();
    std::filesystem::create_directories(root);

    const auto bomPath = root / "bom.ini";
    const std::string bomSource =
        "\xEF\xBB\xBFversion=2\n[State]\nvalue=ready\n";
    writeFile(bomPath, bomSource);
    Document document;
    CMS_TEST_REQUIRE(loadPath(bomPath, document) == Status::ok);
    CMS_TEST_CHECK(document.getValue("", "version") == "2");
    CMS_TEST_CHECK(document.getValue("State", "value") == "ready");
    CMS_TEST_REQUIRE(savePath(bomPath, document) == Status::ok);
    CMS_TEST_CHECK(readFile(bomPath) == "version=2\n[State]\nvalue=ready\n");

    const auto bomOnlyPath = root / "bom-only.ini";
    writeFile(bomOnlyPath, "\xEF\xBB\xBF");
    Document bomOnly;
    CMS_TEST_REQUIRE(loadPath(bomOnlyPath, bomOnly) == Status::ok);
    CMS_TEST_REQUIRE(savePath(bomOnlyPath, bomOnly) == Status::ok);
    CMS_TEST_CHECK(readFile(bomOnlyPath).empty());

    const auto embeddedBomPath = root / "embedded-bom.ini";
    writeFile(embeddedBomPath, "[State]\nvalue=left\xEF\xBB\xBFright\n");
    Document embeddedBom;
    CMS_TEST_REQUIRE(loadPath(embeddedBomPath, embeddedBom) == Status::ok);
    CMS_TEST_CHECK(
        embeddedBom.getValue("State", "value") ==
        "left\xEF\xBB\xBFright");

    const auto invalidBomPath = root / "invalid-bom.ini";
    writeFile(invalidBomPath, "\xEF\xBB\xBF[State]\nvalue=\xFF\n");
    Document stable;
    stable["State"]["value"] = "stable";
    CMS_TEST_CHECK(
        loadPath(invalidBomPath, stable) == Status::invalid_utf8);
    CMS_TEST_CHECK(stable.getValue("State", "value") == "stable");
}

} // namespace

int main() {
    testReadWriteAndMissingValues();
    testGlobalAndProxyLifetime();
    testDuplicateUpdateUsesExistingSemantics();
    testConstReadOnlyProxy();
    testHostAdapterFailureIsTransactional();
    testParsingAndPreservation();
    testInvalidFileArguments();
    testTextModesAndUtf8Detection();
    testBomHandling();
    cleanup();
    return cms::test::finish();
}
