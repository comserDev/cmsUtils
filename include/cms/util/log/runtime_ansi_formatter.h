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

    // 기본값은 ANSI color를 사용하는 V1 호환 동작이다.
    RuntimeAnsiFormatter() noexcept = default;

    // 이후 format 호출에서 ANSI color를 사용할지 설정한다.
    void setUseColor(bool enabled) noexcept {
        useColor_ = enabled;
    }

    // 현재 color 설정을 반환한다.
    bool useColor() const noexcept {
        return useColor_;
    }

    // 설정에 따라 ANSI 또는 plain format을 transactional하게 기록한다.
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
