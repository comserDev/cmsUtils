#pragma once

#include <cstddef>

#include <cms/util/log/ansi_formatter.h>
#include <cms/util/log/formatter.h>
#include <cms/util/log/record.h>
#include <cms/util/status.h>
#include <cms/util/string_buffer.h>

namespace cms {
namespace util {
namespace log {

// setUseColor, useColor, format을 동시에 호출하려면 caller가 외부에서 동기화한다.
class RuntimeAnsiFormatter {
public:
    static constexpr std::size_t maxOverhead =
        maxAnsiFormattedRecordOverhead;

    RuntimeAnsiFormatter() noexcept = default;

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
            ? formatAnsi(record, output)
            : cms::util::log::format(record, output);
    }

private:
    // V1 LoggerBase와 같은 기본 동작으로 ANSI color를 활성화한다.
    bool useColor_ = true;
};

} // namespace log
} // namespace util
} // namespace cms
