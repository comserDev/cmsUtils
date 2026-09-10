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

// 파일을 읽어 document에 반영한다. 입력 검증이나 parsing에 실패하면 기존
// document를 유지하며, mode가 automatic이면 BOM과 UTF-8 유효성을 기준으로
// text mode를 선택한다.
Status loadFile(
    const char* path,
    Document& document,
    TextMode mode = TextMode::automatic);
// document를 원본 줄 구조와 현재 값으로 파일에 저장한다.
Status saveFile(const char* path, const Document& document);
// fs::FS에서 INI를 읽어 document에 반영한다. 실패 시 기존 document를 유지한다.
Status loadFile(
    fs::FS& filesystem,
    const char* path,
    Document& document,
    TextMode mode = TextMode::automatic);
// document를 fs::FS의 파일에 저장한다.
Status saveFile(
    fs::FS& filesystem,
    const char* path,
    const Document& document);

// INI의 section/key 값과 원본 줄 구조를 함께 보존하는 문서 모델이다.
// 주석, 빈 줄, unknown line, 중복 key와 줄바꿈 형식을 유지하며 내부적으로
// std::string과 std::vector 등을 사용하므로 deterministic zero-heap 타입은 아니다.
class Document {
public:
    class SectionProxy;
    class ConstSectionProxy;
    class ValueProxy;
    class ConstValueProxy;

    Document() = default;

    // 모든 section/key와 원본 줄을 제거하고 빈 문서 상태로 되돌린다.
    void clear();

    // 값을 조회한다. key가 없으면 defaultValue를 반환하며 문서를 수정하지 않는다.
    std::string getValue(
        const std::string& section,
        const std::string& key,
        const std::string& defaultValue = {}) const;

    // 값을 갱신하거나 새 key를 문서에 삽입한다. 중복 key가 있으면 마지막 항목을 갱신한다.
    Status setValue(
        const std::string& section,
        const std::string& key,
        const std::string& value);

    // 지정한 section/key가 문서에 존재하는지 확인한다.
    bool contains(
        const std::string& section,
        const std::string& key) const;

    // 수정 가능한 proxy를 반환한다. 실제 section/key 생성은 assignment 시점에만 일어난다.
    SectionProxy operator[](const std::string& section);
    // const 문서에서 읽기 전용 proxy를 반환한다.
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

// Document와 section/key 이름만 보관하는 수정 proxy다. vector element pointer를
// 저장하지 않으므로 다른 key 삽입으로 내부 storage가 재배치되어도 dangling되지 않는다.
class Document::ValueProxy {
public:
    ValueProxy(Document* document, std::string section, std::string key)
        : document_(document),
          section_(std::move(section)),
          key_(std::move(key)) {}

    // 기존 key를 갱신하거나 없는 section/key를 생성한다.
    ValueProxy& operator=(const std::string& value);
    ValueProxy& operator=(const char* value);

    operator std::string() const;

    // 없는 key는 빈 문자열을 반환하며 조회만으로 문서를 수정하지 않는다.
    std::string get() const;
    bool exists() const;

private:
    Document* document_;
    std::string section_;
    std::string key_;
};

// const Document에서 section/key를 조회하는 읽기 전용 proxy다.
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

// 하나의 section 이름을 기준으로 ValueProxy를 만드는 수정 proxy다.
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

// const Document에서 하나의 section을 조회하는 읽기 전용 proxy다.
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
