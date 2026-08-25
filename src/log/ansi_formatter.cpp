#include <cms/log/ansi_formatter.h>

#include <cstddef>
#include <cstring>
#include <limits>

#include <cms/format.h>
#include <cms/log/formatter.h>
#include <cms/static_string.h>
#include <cms/string_view.h>

namespace cms {
namespace log {

namespace {

constexpr StringView ansiReset() noexcept {
    return StringView("\033[0m");
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
        destination[offset] = value[index];
        ++offset;
    }
}

} // namespace

WriteResult formatAnsi(
    const Record& record,
    StringBuffer output) noexcept {
    if (!output.valid()) {
        return {Status::invalid_argument, 0, 0};
    }

    StaticString<21> timestamp;
    const WriteResult timestampResult = cms::format::unsignedInteger(
        record.timestampMilliseconds,
        timestamp.buffer());
    if (timestampResult.status != Status::ok) {
        return {timestampResult.status, 0, timestampResult.required};
    }

    const StringView name = levelName(record.level);
    const StringView color = levelColor(record.level);
    const StringView reset = ansiReset();
    const std::size_t fixedBytes = 7;
    std::size_t required = 0;
    if (!addSize(required, timestamp.size(), required)
        || !addSize(required, color.size(), required)
        || !addSize(required, name.size(), required)
        || !addSize(required, reset.size(), required)
        || !addSize(required, record.message.size(), required)
        || !addSize(required, fixedBytes, required)) {
        return {Status::out_of_range, 0, 0};
    }

    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    const std::size_t messageOffset =
        timestamp.size() + color.size() + name.size()
        + reset.size() + fixedBytes - 1;
    if (!record.message.empty()) {
        // output과 message가 겹쳐도 prefix가 원본을 덮지 않도록 먼저 옮긴다.
        std::memmove(
            output.data() + messageOffset,
            record.message.data(),
            record.message.size());
    }

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
    offset += record.message.size();
    output.data()[offset++] = '\n';

    const Status committed = output.commit(offset);
    if (committed != Status::ok) {
        return {committed, 0, 0};
    }
    return {Status::ok, required, required};
}

} // namespace log
} // namespace cms
