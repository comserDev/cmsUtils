#pragma once

#include <cstddef>

#include <cms/log/level.h>
#include <cms/log/record.h>
#include <cms/status.h>
#include <cms/string_buffer.h>
#include <cms/string_view.h>

namespace cms {
namespace log {

// 최대 timestamp와 CRITICAL level을 포함한 message 외 payload 크기다.
inline constexpr std::size_t maxFormattedRecordOverhead = 35;

StringView levelName(Level level) noexcept;

// "[timestamp] [LEVEL] message\n" 전체를 transactional하게 기록한다.
WriteResult format(const Record& record, StringBuffer output) noexcept;

} // namespace log
} // namespace cms
