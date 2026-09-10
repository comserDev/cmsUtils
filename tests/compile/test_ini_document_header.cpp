#include <type_traits>
#include <utility>

#include <cms/util/ini/document.h>
#include <cms/util/ini/file.h>

using Document = cms::util::ini::Document;
using TextMode = cms::util::ini::TextMode;

using ConstSection = decltype(std::declval<const Document&>()[std::string{}]);
using ConstValue = decltype(std::declval<ConstSection>()[std::string{}]);

static_assert(
    !std::is_assignable<ConstValue&, const char*>::value,
    "const Document proxies must not be assignable");

int iniDocumentHeaderCompile() {
    Document document;
    document["Section"]["key"] = "value";
    (void)cms::util::ini::loadFile("config.ini", document);
    (void)cms::util::ini::loadFile(
        "config.ini",
        document,
        TextMode::utf8);
    (void)cms::util::ini::saveFile("config.ini", document);
    return document["Section"]["key"].exists() ? 0 : 0;
}
