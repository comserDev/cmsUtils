#pragma once

#include <cstddef>

#include <cms/util/log/level.h>
#include <cms/util/log/record.h>
#include <cms/util/status.h>
#include <cms/util/string_buffer.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

// 최대 timestamp와 CRITICAL level을 포함한 message 외 payload 크기다.
inline constexpr std::size_t maxFormattedRecordOverhead = 35;

// 알려진 level의 이름을 반환하며 unknown 값은 안전한 진단 문자열로 표현한다.
StringView levelName(Level level) noexcept;

// "[timestamp] [LEVEL] message\n" 전체를 transactional하게 기록한다.
WriteResult format(const Record& record, StringBuffer output) noexcept;

// PlainFormatter는 기본 text formatter를 정책 타입으로 노출한다.
struct PlainFormatter {
    static constexpr std::size_t maxOverhead =
        maxFormattedRecordOverhead;

    static WriteResult format(
        const Record& record,
        StringBuffer output) noexcept {
        return cms::util::log::format(record, output);
    }
};

} // namespace log
} // namespace util
} // namespace cms
