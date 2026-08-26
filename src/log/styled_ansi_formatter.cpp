#include <cms/util/log/styled_ansi_formatter.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include <cms/util/format.h>
#include <cms/util/log/level.h>
#include <cms/util/static_string.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

namespace {

constexpr StringView ansiReset() noexcept {
    return StringView("\033[0m");
}

constexpr StringView keywordColor() noexcept {
    return StringView("\033[1;91m");
}

StringView levelColor(Level level) noexcept {
    switch (level) {
    case Level::debug:
        return StringView("\033[36m");
    case Level::info:
        return StringView("\033[32m");
    case Level::warning:
        return StringView("\033[33m");
    case Level::error:
        return StringView("\033[31m");
    default:
        return StringView();
    }
}

bool addSize(
    std::size_t current,
    std::size_t increment,
    std::size_t& result) noexcept {
    if (increment > (std::numeric_limits<std::size_t>::max)() - current) {
        return false;
    }

    result = current + increment;
    return true;
}

void writeBytes(
    char* destination,
    std::size_t& offset,
    StringView value) noexcept {
    for (std::size_t index = 0; index < value.size(); ++index) {
        destination[offset++] = value[index];
    }
}

constexpr unsigned char asciiUpper(unsigned char value) noexcept {
    if (value >= static_cast<unsigned char>('a')
        && value <= static_cast<unsigned char>('z')) {
        return static_cast<unsigned char>(value - ('a' - 'A'));
    }
    return value;
}

StringView tagColor(StringView tagBody) noexcept {
    static constexpr StringView palette[] = {
        StringView("\033[92m"),
        StringView("\033[93m"),
        StringView("\033[94m"),
        StringView("\033[95m"),
        StringView("\033[96m"),
        StringView("\033[32m"),
        StringView("\033[33m"),
        StringView("\033[35m"),
        StringView("\033[36m")};

    // V1의 32-bit unsigned int DJB2 결과를 target ABI와 무관하게 고정한다.
    std::uint32_t hash = 5381U;
    for (std::size_t index = 0; index < tagBody.size(); ++index) {
        const unsigned char value = asciiUpper(
            static_cast<unsigned char>(tagBody[index]));
        hash = ((hash << 5U) + hash) + value;
    }
    return palette[hash % (sizeof(palette) / sizeof(palette[0]))];
}

constexpr bool asciiEqualIgnoreCase(char left, char right) noexcept {
    return asciiUpper(static_cast<unsigned char>(left))
        == asciiUpper(static_cast<unsigned char>(right));
}

bool startsWithIgnoreCase(
    StringView value,
    std::size_t offset,
    StringView keyword) noexcept {
    if (offset > value.size() || keyword.size() > value.size() - offset) {
        return false;
    }

    for (std::size_t index = 0; index < keyword.size(); ++index) {
        if (!asciiEqualIgnoreCase(value[offset + index], keyword[index])) {
            return false;
        }
    }
    return true;
}

StringView matchingKeyword(StringView message, std::size_t offset) noexcept {
    static constexpr StringView keywords[] = {
        StringView("ERROR"),
        StringView("CRITICAL"),
        StringView("FATAL"),
        StringView("FAIL")};

    for (const StringView keyword : keywords) {
        if (startsWithIgnoreCase(message, offset, keyword)) {
            return keyword;
        }
    }
    return StringView();
}

std::size_t closingBracket(StringView message, std::size_t open) noexcept {
    for (std::size_t index = open + 1; index < message.size(); ++index) {
        if (message[index] == ']') {
            return index;
        }
    }
    return message.size();
}

bool styledMessageSize(StringView message, std::size_t& result) noexcept {
    const std::size_t tagExtra = 9;
    const std::size_t keywordExtra = 11;
    std::size_t required = message.size();

    for (std::size_t offset = 0; offset < message.size();) {
        if (message[offset] == '[') {
            const std::size_t close = closingBracket(message, offset);
            if (close < message.size() && close > offset + 1) {
                if (!addSize(required, tagExtra, required)) {
                    return false;
                }
                offset = close + 1;
                continue;
            }
            // 닫는 괄호가 없으면 이후에도 valid tag가 없으므로 다시 찾지 않는다.
            if (close == message.size()) {
                for (; offset < message.size();) {
                    const StringView keyword = matchingKeyword(message, offset);
                    if (!keyword.empty()) {
                        if (!addSize(required, keywordExtra, required)) {
                            return false;
                        }
                        offset += keyword.size();
                    } else {
                        ++offset;
                    }
                }
                break;
            }
        }

        const StringView keyword = matchingKeyword(message, offset);
        if (!keyword.empty()) {
            if (!addSize(required, keywordExtra, required)) {
                return false;
            }
            offset += keyword.size();
        } else {
            ++offset;
        }
    }

    result = required;
    return true;
}

void writeStyledMessage(
    char* destination,
    std::size_t& outputOffset,
    StringView message) noexcept {
    const StringView reset = ansiReset();
    bool scanTags = true;

    for (std::size_t offset = 0; offset < message.size();) {
        if (scanTags && message[offset] == '[') {
            const std::size_t close = closingBracket(message, offset);
            if (close < message.size() && close > offset + 1) {
                writeBytes(
                    destination,
                    outputOffset,
                    tagColor(StringView(
                        message.data() + offset + 1,
                        close - offset - 1)));
                for (; offset <= close; ++offset) {
                    destination[outputOffset++] = message[offset];
                }
                writeBytes(destination, outputOffset, reset);
                continue;
            }
            if (close == message.size()) {
                scanTags = false;
            }
        }

        const StringView keyword = matchingKeyword(message, offset);
        if (!keyword.empty()) {
            writeBytes(destination, outputOffset, keywordColor());
            for (std::size_t index = 0; index < keyword.size(); ++index) {
                destination[outputOffset++] = message[offset + index];
            }
            writeBytes(destination, outputOffset, reset);
            offset += keyword.size();
        } else {
            destination[outputOffset++] = message[offset++];
        }
    }
}

} // namespace

WriteResult formatStyledAnsi(
    const Record& record,
    StringBuffer output) noexcept {
    if (!output.valid()) {
        return {Status::invalid_argument, 0, 0};
    }

    StaticString<21> timestamp;
    const WriteResult timestampResult = cms::util::format::unsignedInteger(
        record.timestampMilliseconds,
        timestamp.buffer());
    if (timestampResult.status != Status::ok) {
        return {timestampResult.status, 0, timestampResult.required};
    }

    const StringView name = levelName(record.level);
    const StringView color = levelColor(record.level);
    const StringView reset = ansiReset();
    const std::size_t fixedBytes = 7;
    std::size_t fixedRequired = 0;
    if (!addSize(fixedRequired, timestamp.size(), fixedRequired)
        || !addSize(fixedRequired, color.size(), fixedRequired)
        || !addSize(fixedRequired, name.size(), fixedRequired)
        || !addSize(fixedRequired, reset.size(), fixedRequired)
        || !addSize(fixedRequired, fixedBytes, fixedRequired)
        || record.message.size()
            > (std::numeric_limits<std::size_t>::max)() - fixedRequired) {
        return {Status::out_of_range, 0, 0};
    }

    std::size_t styledBytes = 0;
    if (!styledMessageSize(record.message, styledBytes)) {
        return {Status::out_of_range, 0, 0};
    }

    std::size_t required = 0;
    if (!addSize(fixedRequired, styledBytes, required)) {
        return {Status::out_of_range, 0, 0};
    }

    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    // 원본을 최종 message 영역의 끝에 맞춰 옮기면 확장하며 앞에서 써도
    // 아직 읽지 않은 byte를 덮지 않는다. input/output overlap도 memmove가 처리한다.
    const std::size_t sourceOffset = required - 1 - record.message.size();
    if (!record.message.empty()) {
        std::memmove(
            output.data() + sourceOffset,
            record.message.data(),
            record.message.size());
    }
    const StringView source(
        output.data() + sourceOffset,
        record.message.size());

    std::size_t offset = 0;
    output.data()[offset++] = '[';
    writeBytes(output.data(), offset, timestamp.view());
    output.data()[offset++] = ']';
    output.data()[offset++] = ' ';
    if (color.empty()) {
        writeBytes(output.data(), offset, reset);
    } else {
        writeBytes(output.data(), offset, color);
    }
    output.data()[offset++] = '[';
    writeBytes(output.data(), offset, name);
    output.data()[offset++] = ']';
    if (!color.empty()) {
        writeBytes(output.data(), offset, reset);
    }
    output.data()[offset++] = ' ';
    writeStyledMessage(output.data(), offset, source);
    output.data()[offset++] = '\n';

    const Status committed = output.commit(offset);
    if (committed != Status::ok) {
        return {committed, 0, 0};
    }
    return {Status::ok, required, required};
}

} // namespace log
} // namespace util
} // namespace cms
