#include <cms/util/ini/document.h>

#include <cms/util/string_ops.h>
#include <cms/util/utf8.h>

#include <utility>

namespace cms {
namespace util {
namespace ini {
namespace {

StringView trimValue(StringView value, TextMode mode) noexcept {
    return mode == TextMode::utf8
        ? cms::util::utf8::trim(value)
        : cms::util::string::trim(value);
}

std::string toString(StringView value) {
    return std::string(value.data(), value.size());
}

bool hasUtf8Bom(const std::string& value) noexcept {
    return value.size() >= 3
        && static_cast<unsigned char>(value[0]) == 0xEFU
        && static_cast<unsigned char>(value[1]) == 0xBBU
        && static_cast<unsigned char>(value[2]) == 0xBFU;
}

} // namespace

void Document::parseDocument(
    const std::string& content,
    std::vector<LineEntry>& lines,
    std::string& newline,
    bool& finalNewline,
    TextMode mode) {
    // 줄 원문은 보존하고 metadata만 구성해 save 시 주석과 unknown line을 유지한다.
    newline = "\n";
    for (std::size_t index = 0; index < content.size(); ++index) {
        if (content[index] != '\n') continue;
        newline = index > 0 && content[index - 1] == '\r'
            ? "\r\n"
            : "\n";
        break;
    }
    finalNewline = !content.empty() && content.back() == '\n';

    lines.clear();
    std::string currentSection;
    std::size_t position = 0;
    while (position < content.size()) {
        const std::size_t lineEnd = content.find('\n', position);
        const std::size_t end = lineEnd == std::string::npos
            ? content.size()
            : lineEnd;
        std::string text = content.substr(position, end - position);
        if (!text.empty() && text.back() == '\r') {
            text.pop_back();
        }

        LineEntry entry;
        entry.text = text;
        const std::string trimmed = toString(trimValue(
            StringView(text.data(), text.size()),
            mode));
        if (trimmed.empty()) {
            entry.kind = LineKind::blank;
        } else if (trimmed.front() == ';' || trimmed.front() == '#') {
            entry.kind = LineKind::comment;
        } else if (trimmed.front() == '[' && trimmed.back() == ']') {
            const std::string section = toString(trimValue(
                StringView(
                    trimmed.data() + 1,
                    trimmed.size() - 2),
                mode));
            if (!section.empty()) {
                entry.kind = LineKind::section;
                entry.section = section;
                currentSection = section;
            }
        } else {
            const std::size_t separator = trimmed.find('=');
            if (separator != std::string::npos) {
                const std::string key = toString(trimValue(
                    StringView(trimmed.data(), separator),
                    mode));
                if (!key.empty()) {
                    entry.kind = LineKind::key;
                    entry.section = currentSection;
                    entry.key = key;
                }
            }
        }
        lines.push_back(std::move(entry));
        if (lineEnd == std::string::npos) break;
        position = lineEnd + 1;
    }
}

void Document::rebuildIndex() {
    // line insertion으로 vector index가 바뀔 수 있으므로 lookup metadata를 재생성한다.
    keyIndex_.clear();
    sectionIndex_.clear();
    for (std::size_t index = 0; index < lines_.size(); ++index) {
        const LineEntry& line = lines_[index];
        if (line.kind == LineKind::section) {
            sectionIndex_[line.section].push_back(index);
        } else if (line.kind == LineKind::key) {
            keyIndex_[LookupKey{line.section, line.key}].push_back(index);
        }
    }
}

void Document::clear() {
    newline_ = "\n";
    finalNewline_ = false;
    textMode_ = TextMode::automatic;
    lines_.clear();
    keyIndex_.clear();
    sectionIndex_.clear();
}

Status Document::replaceFromText(
    const std::string& content,
    TextMode mode) {
    // parsing 결과를 임시 container에 완성한 뒤 성공할 때만 swap해 transactional load를 보장한다.
    std::size_t contentOffset = 0;
    TextMode effectiveMode = mode;
    const bool bom = hasUtf8Bom(content);
    if (bom && mode != TextMode::ascii) {
        contentOffset = 3;
        effectiveMode = TextMode::utf8;
    }

    const StringView input(
        content.data() + contentOffset,
        content.size() - contentOffset);
    if (mode == TextMode::utf8 || (bom && mode != TextMode::ascii)) {
        if (cms::util::utf8::validate(input) != Status::ok) {
            return Status::invalid_utf8;
        }
    } else if (mode == TextMode::automatic) {
        effectiveMode = cms::util::utf8::validate(input) == Status::ok
            ? TextMode::utf8
            : TextMode::ascii;
    } else {
        effectiveMode = TextMode::ascii;
    }

    const std::string parsedContent = content.substr(contentOffset);
    std::vector<LineEntry> parsedLines;
    std::string parsedNewline;
    bool parsedFinalNewline = false;
    parseDocument(
        parsedContent,
        parsedLines,
        parsedNewline,
        parsedFinalNewline,
        effectiveMode);

    lines_.swap(parsedLines);
    newline_ = std::move(parsedNewline);
    finalNewline_ = parsedFinalNewline;
    textMode_ = effectiveMode;
    rebuildIndex();
    return Status::ok;
}

std::string Document::serialize() const {
    // 보존한 줄 순서와 newline/final-newline 정책을 사용해 원본 구조를 재구성한다.
    std::string result;
    for (std::size_t index = 0; index < lines_.size(); ++index) {
        if (index != 0) result += newline_;
        result += lines_[index].text;
    }
    if (finalNewline_ && !lines_.empty()) result += newline_;
    return result;
}

std::string Document::getValue(
    const std::string& section,
    const std::string& key,
    const std::string& defaultValue) const {
    const auto found = keyIndex_.find(LookupKey{section, key});
    if (found == keyIndex_.end() || found->second.empty()) {
        return defaultValue;
    }
    const std::size_t lineIndex = found->second.back();
    const std::string& text = lines_[lineIndex].text;
    const std::size_t separator = text.find('=');
    if (separator == std::string::npos) return defaultValue;
    return toString(trimValue(
        StringView(
            text.data() + separator + 1,
            text.size() - separator - 1),
        textMode_));
}

bool Document::contains(
    const std::string& section,
    const std::string& key) const {
    const auto found = keyIndex_.find(LookupKey{section, key});
    return found != keyIndex_.end() && !found->second.empty();
}

Status Document::setValue(
    const std::string& section,
    const std::string& key,
    const std::string& value) {
    // 기존 마지막 duplicate를 갱신하거나 section 경계를 유지한 위치에 새 line을 삽입한다.
    if (key.empty()) return Status::invalid_argument;

    const LookupKey lookup{section, key};
    const auto existing = keyIndex_.find(lookup);
    if (existing != keyIndex_.end() && !existing->second.empty()) {
        const std::size_t lineIndex = existing->second.back();
        lines_[lineIndex].text = key + " = " + value;
        rebuildIndex();
        return Status::ok;
    }

    LineEntry entry;
    entry.text = key + " = " + value;
    entry.kind = LineKind::key;
    entry.section = section;
    entry.key = key;

    if (section.empty()) {
        std::size_t insertion = lines_.size();
        for (std::size_t index = 0; index < lines_.size(); ++index) {
            if (lines_[index].kind == LineKind::section) {
                insertion = index;
                break;
            }
        }
        lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(insertion),
                      std::move(entry));
    } else {
        const auto sectionFound = sectionIndex_.find(section);
        if (sectionFound == sectionIndex_.end() || sectionFound->second.empty()) {
            LineEntry header;
            header.text = "[" + section + "]";
            header.kind = LineKind::section;
            header.section = section;
            lines_.push_back(std::move(header));
            lines_.push_back(std::move(entry));
        } else {
            const std::size_t lastHeader = sectionFound->second.back();
            std::size_t insertion = lines_.size();
            for (std::size_t index = lastHeader + 1;
                 index < lines_.size(); ++index) {
                if (lines_[index].kind == LineKind::section) {
                    insertion = index;
                    break;
                }
            }
            lines_.insert(
                lines_.begin() + static_cast<std::ptrdiff_t>(insertion),
                std::move(entry));
        }
    }
    rebuildIndex();
    return Status::ok;
}

Document::ValueProxy& Document::ValueProxy::operator=(
    const std::string& value) {
    document_->setValue(section_, key_, value);
    return *this;
}

Document::ValueProxy& Document::ValueProxy::operator=(const char* value) {
    document_->setValue(section_, key_, value == nullptr ? "" : value);
    return *this;
}

Document::ValueProxy::operator std::string() const {
    return get();
}

std::string Document::ValueProxy::get() const {
    return document_->getValue(section_, key_);
}

bool Document::ValueProxy::exists() const {
    return document_->contains(section_, key_);
}

Document::ConstValueProxy::operator std::string() const {
    return get();
}

std::string Document::ConstValueProxy::get() const {
    return document_->getValue(section_, key_);
}

bool Document::ConstValueProxy::exists() const {
    return document_->contains(section_, key_);
}

Document::ValueProxy Document::SectionProxy::operator[](
    const std::string& key) {
    return ValueProxy(document_, section_, key);
}

bool Document::SectionProxy::contains(const std::string& key) const {
    return document_->contains(section_, key);
}

Document::ConstValueProxy Document::ConstSectionProxy::operator[](
    const std::string& key) const {
    return ConstValueProxy(document_, section_, key);
}

bool Document::ConstSectionProxy::contains(const std::string& key) const {
    return document_->contains(section_, key);
}

Document::SectionProxy Document::operator[](
    const std::string& section) {
    return SectionProxy(this, section);
}

Document::ConstSectionProxy Document::operator[](
    const std::string& section) const {
    return ConstSectionProxy(this, section);
}

} // namespace ini
} // namespace util
} // namespace cms
