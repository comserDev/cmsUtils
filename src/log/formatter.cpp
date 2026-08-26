#include <cms/util/log/formatter.h>

#include <cstddef>
#include <cstring>
#include <limits>

#include <cms/util/format.h>
#include <cms/util/static_string.h>

namespace cms {
namespace util {
namespace log {

namespace {

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

StringView levelName(Level level) noexcept {
    switch (level) {
    case Level::trace:
        return StringView("TRACE");
    case Level::debug:
        return StringView("DEBUG");
    case Level::info:
        return StringView("INFO");
    case Level::warning:
        return StringView("WARNING");
    case Level::error:
        return StringView("ERROR");
    case Level::critical:
        return StringView("CRITICAL");
    default:
        return StringView("UNKNOWN");
    }
}

WriteResult format(const Record& record, StringBuffer output) noexcept {
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
    std::size_t required = 0;
    const std::size_t fixedBytes = 7;
    if (!addSize(required, timestamp.size(), required)
        || !addSize(required, name.size(), required)
        || !addSize(required, record.message.size(), required)
        || !addSize(required, fixedBytes, required)) {
        return {Status::out_of_range, 0, 0};
    }

    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    const std::size_t messageOffset =
        timestamp.size() + name.size() + fixedBytes - 1;
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
    output.data()[offset++] = '[';
    writeBytes(output.data(), offset, name);
    output.data()[offset++] = ']';
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
} // namespace util
} // namespace cms
