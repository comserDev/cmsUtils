#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <cms/log/formatter.h>
#include <cms/log/record.h>
#include <cms/log/runtime_ansi_formatter.h>
#include <cms/log/styled_ansi_formatter.h>
#include <cms/static_string.h>
#include <cms/status.h>
#include <cms/string_buffer.h>

namespace cms {
namespace log {

inline constexpr int minUtcOffsetMinutes = -12 * 60;
inline constexpr int maxUtcOffsetMinutes = 14 * 60;
inline constexpr std::size_t utcOffsetTimestampSize = 8;

// Unix epoch milliseconds에 fixed UTC offset을 적용해 "HH:MM:SS"를 기록한다.
// 날짜와 millisecond는 출력하지 않으며 timezone이나 DST를 자동 계산하지 않는다.
WriteResult formatUtcOffsetTimestamp(
    Timestamp unixEpochMilliseconds,
    int utcOffsetMinutes,
    StringBuffer output) noexcept;

namespace detail {

template<class Formatter>
struct IsSupportedUtcOffsetBase : std::false_type {};

template<>
struct IsSupportedUtcOffsetBase<PlainFormatter> : std::true_type {};

template<>
struct IsSupportedUtcOffsetBase<AnsiFormatter> : std::true_type {};

template<>
struct IsSupportedUtcOffsetBase<RuntimeAnsiFormatter> : std::true_type {};

template<>
struct IsSupportedUtcOffsetBase<StyledAnsiFormatter> : std::true_type {};

template<>
struct IsSupportedUtcOffsetBase<RuntimeStyledAnsiFormatter>
    : std::true_type {};

template<class Formatter>
WriteResult formatUtcOffsetRecord(
    const Formatter& formatter,
    int utcOffsetMinutes,
    const Record& record,
    StringBuffer output) noexcept(noexcept(
        formatter.format(record, output))) {
    StaticString<utcOffsetTimestampSize + 1> timestamp;
    const WriteResult timestampResult = formatUtcOffsetTimestamp(
        record.timestampMilliseconds,
        utcOffsetMinutes,
        timestamp.buffer());
    if (timestampResult.status != Status::ok) {
        return timestampResult;
    }

    // 여덟 자리 placeholder를 사용해 기존 formatter의 capacity 계산을 그대로 쓴다.
    constexpr Timestamp placeholder = 10000000;
    const Record formattedRecord{
        record.level,
        placeholder,
        record.message};
    const WriteResult result = formatter.format(formattedRecord, output);
    if (result.status != Status::ok) {
        return result;
    }

    for (std::size_t index = 0; index < utcOffsetTimestampSize; ++index) {
        output.data()[index + 1] = timestamp.data()[index];
    }
    return result;
}

} // namespace detail

// 기존 line formatter의 raw timestamp만 client-supplied fixed offset 시각으로 바꾼다.
// offset 기본값은 UTC이며 마지막 성공 설정을 다음 setter 호출까지 유지한다.
// client가 timezone과 DST를 결정하며 queued record에도 format 시점 offset을 적용한다.
// Formatter는 다섯 built-in formatter만 지원하며 timestamp prefix contract를 공유한다.
// setter와 format을 동시에 호출하려면 caller가 동기화해야 한다.
template<class Formatter = PlainFormatter>
class UtcOffsetFormatter : public Formatter {
    static_assert(
        detail::IsSupportedUtcOffsetBase<Formatter>::value,
        "UtcOffsetFormatter supports only built-in log formatters");

public:
    using Formatter::Formatter;

    Status setUtcOffsetMinutes(int minutes) noexcept {
        if (minutes < minUtcOffsetMinutes
            || minutes > maxUtcOffsetMinutes) {
            return Status::invalid_argument;
        }

        utcOffsetMinutes_ = static_cast<std::int16_t>(minutes);
        return Status::ok;
    }

    int utcOffsetMinutes() const noexcept {
        return utcOffsetMinutes_;
    }

    WriteResult format(
        const Record& record,
        StringBuffer output) const noexcept(noexcept(
            std::declval<const Formatter&>().format(record, output))) {
        return detail::formatUtcOffsetRecord(
            static_cast<const Formatter&>(*this),
            utcOffsetMinutes_,
            record,
            output);
    }

private:
    std::int16_t utcOffsetMinutes_ = 0;
};

} // namespace log
} // namespace cms
