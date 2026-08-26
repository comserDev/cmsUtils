#pragma once

#include <cstddef>

#include <cms/util/log/record.h>
#include <cms/util/status.h>
#include <cms/util/string_buffer.h>

namespace cms {
namespace util {
namespace log {

// V1과 같이 level badge만 색칠하고 badge 직후 reset한다.
// 색상 mapping이 없던 level은 badge 전에 reset해 외부 ANSI state를 상속하지 않는다.
// 최대 overhead는 timestamp 20, 구분자 7, WARNING 7, color 5, reset 4다.
inline constexpr std::size_t maxAnsiFormattedRecordOverhead = 43;

WriteResult formatAnsi(
    const Record& record,
    StringBuffer output) noexcept;

struct AnsiFormatter {
    static constexpr std::size_t maxOverhead =
        maxAnsiFormattedRecordOverhead;

    static WriteResult format(
        const Record& record,
        StringBuffer output) noexcept {
        return formatAnsi(record, output);
    }
};

} // namespace log
} // namespace util
} // namespace cms
