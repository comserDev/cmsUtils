#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cms/util/status.h>

namespace fs {
class FS;
}

namespace cms {
namespace util {
namespace ini {

enum class TextMode {
    automatic,
    ascii,
    utf8
};

class Document;

Status loadFile(
    const char* path,
    Document& document,
    TextMode mode = TextMode::automatic);
Status saveFile(const char* path, const Document& document);
Status loadFile(
    fs::FS& filesystem,
    const char* path,
    Document& document,
    TextMode mode = TextMode::automatic);
Status saveFile(
    fs::FS& filesystem,
    const char* path,
    const Document& document);

class Document {
public:
    class SectionProxy;
    class ConstSectionProxy;
    class ValueProxy;
    class ConstValueProxy;

    Document() = default;

    void clear();

    std::string getValue(
        const std::string& section,
        const std::string& key,
        const std::string& defaultValue = {}) const;

    Status setValue(
        const std::string& section,
        const std::string& key,
        const std::string& value);

    bool contains(
        const std::string& section,
        const std::string& key) const;

    SectionProxy operator[](const std::string& section);
    ConstSectionProxy operator[](const std::string& section) const;

private:
    enum class LineKind {
        blank,
        comment,
        section,
        key,
        unknown
    };

    struct LineEntry {
        std::string text;
        LineKind kind = LineKind::unknown;
        std::string section;
        std::string key;
    };

    struct LookupKey {
        std::string section;
        std::string key;

        bool operator==(const LookupKey& other) const noexcept {
            return section == other.section && key == other.key;
        }
    };

    struct LookupKeyHash {
        std::size_t operator()(const LookupKey& value) const noexcept {
            const std::size_t sectionHash = std::hash<std::string>{}(
                value.section);
            const std::size_t keyHash = std::hash<std::string>{}(value.key);
            return sectionHash ^ (keyHash + static_cast<std::size_t>(
                0x9E3779B9U) + (sectionHash << 6U) + (sectionHash >> 2U));
        }
    };

    static void parseDocument(
        const std::string& content,
        std::vector<LineEntry>& lines,
        std::string& newline,
        bool& finalNewline,
        TextMode mode);

    void rebuildIndex();
    Status replaceFromText(
        const std::string& content,
        TextMode mode = TextMode::automatic);
    std::string serialize() const;

    friend Status loadFile(
        const char* path,
        Document& document,
        TextMode mode);
    friend Status saveFile(const char* path, const Document& document);
    friend Status loadFile(
        fs::FS& filesystem,
        const char* path,
        Document& document,
        TextMode mode);
    friend Status saveFile(
        fs::FS& filesystem,
        const char* path,
        const Document& document);

    std::string newline_ = "\n";
    bool finalNewline_ = false;
    TextMode textMode_ = TextMode::automatic;
    std::vector<LineEntry> lines_;
    std::unordered_map<LookupKey, std::vector<std::size_t>, LookupKeyHash>
        keyIndex_;
    std::unordered_map<std::string, std::vector<std::size_t>> sectionIndex_;
};

class Document::ValueProxy {
public:
    ValueProxy(Document* document, std::string section, std::string key)
        : document_(document),
          section_(std::move(section)),
          key_(std::move(key)) {}

    ValueProxy& operator=(const std::string& value);
    ValueProxy& operator=(const char* value);

    operator std::string() const;

    std::string get() const;
    bool exists() const;

private:
    Document* document_;
    std::string section_;
    std::string key_;
};

class Document::ConstValueProxy {
public:
    ConstValueProxy(
        const Document* document,
        std::string section,
        std::string key)
        : document_(document),
          section_(std::move(section)),
          key_(std::move(key)) {}

    operator std::string() const;

    std::string get() const;
    bool exists() const;

private:
    const Document* document_;
    std::string section_;
    std::string key_;
};

class Document::SectionProxy {
public:
    SectionProxy(Document* document, std::string section)
        : document_(document), section_(std::move(section)) {}

    ValueProxy operator[](const std::string& key);

    bool contains(const std::string& key) const;

private:
    Document* document_;
    std::string section_;
};

class Document::ConstSectionProxy {
public:
    ConstSectionProxy(const Document* document, std::string section)
        : document_(document), section_(std::move(section)) {}

    ConstValueProxy operator[](const std::string& key) const;

    bool contains(const std::string& key) const;

private:
    const Document* document_;
    std::string section_;
};

} // namespace ini
} // namespace util
} // namespace cms
