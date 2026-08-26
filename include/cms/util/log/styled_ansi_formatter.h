#pragma once

#include <cstddef>

#include <cms/util/log/formatter.h>
#include <cms/util/log/record.h>
#include <cms/util/status.h>
#include <cms/util/string_buffer.h>

namespace cms {
namespace util {
namespace log {

// 가장 짧은 tag인 "[x]"는 원본 3 byte마다 ANSI 9 byte가 추가된다.
inline constexpr std::size_t maxStyledMessageExpansionFactor = 4;

// AsyncLogger의 storage 계산에서 terminating NUL까지 포함한 고정 보정값이다.
inline constexpr std::size_t styledFormattedStorageAdjustment = 40;

// V1의 level, tag hash color, keyword 강조를 한 번에 적용한다.
WriteResult formatStyledAnsi(
    const Record& record,
    StringBuffer output) noexcept;

struct StyledAnsiFormatter {
    static WriteResult format(
        const Record& record,
        StringBuffer output) noexcept {
        return formatStyledAnsi(record, output);
    }
};

// setUseColor, useColor, format을 동시에 호출하려면 caller가 외부에서 동기화한다.
class RuntimeStyledAnsiFormatter {
public:
    RuntimeStyledAnsiFormatter() noexcept = default;

    void setUseColor(bool enabled) noexcept {
        useColor_ = enabled;
    }

    bool useColor() const noexcept {
        return useColor_;
    }

    WriteResult format(
        const Record& record,
        StringBuffer output) const noexcept {
        return useColor_
            ? formatStyledAnsi(record, output)
            : cms::util::log::format(record, output);
    }

private:
    // V1 LoggerBase와 같은 기본 동작으로 전체 ANSI styling을 활성화한다.
    bool useColor_ = true;
};

} // namespace log
} // namespace util
} // namespace cms
