#pragma once

#include <cstdint>

#include <cms/util/log/level.h>

namespace cms {
namespace util {
namespace log {

struct NoLevelFilter {
    static bool allows(Level) noexcept {
        return true;
    }
};

// Runtime 설정과 allows를 동시에 호출하려면 caller가 외부에서 동기화한다.
class RuntimeLevelFilter {
public:
    RuntimeLevelFilter() noexcept = default;

    void setMinLevel(Level level) noexcept {
        // Invalid threshold는 모든 정상 level을 허용하는 trace로 복구한다.
        minLevel_ = isKnown(level) ? level : Level::trace;
    }

    Level minLevel() const noexcept {
        return minLevel_;
    }

    void setEnabled(bool enabled) noexcept {
        enabled_ = enabled;
    }

    bool enabled() const noexcept {
        return enabled_;
    }

    bool allows(Level level) const noexcept {
        if (!enabled_) {
            return false;
        }
        // UNKNOWN 진단은 threshold와 무관하게 formatter까지 전달한다.
        if (!isKnown(level)) {
            return true;
        }
        return static_cast<std::uint8_t>(level)
            >= static_cast<std::uint8_t>(minLevel_);
    }

private:
    static bool isKnown(Level level) noexcept {
        return static_cast<std::uint8_t>(level)
            <= static_cast<std::uint8_t>(Level::critical);
    }

    // V1의 기본 Debug threshold에 대응한다.
    Level minLevel_ = Level::debug;
    bool enabled_ = true;
};

} // namespace log
} // namespace util
} // namespace cms
