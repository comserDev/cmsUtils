#pragma once

#include <chrono>
#include <limits>

#include <cms/log/clock.h>

namespace cms {
namespace platform {

// system_clock epoch이 Unix epoch인 지원 platform/toolchain용 adapter다.
// C++17 자체는 이 epoch를 보장하지 않으며 timezone offset은 적용하지 않는다.
class SystemClock {
public:
    log::Timestamp nowMilliseconds() noexcept {
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch());
        const auto count = elapsed.count();
        if (count <= 0) {
            return 0;
        }

        using Count = decltype(count);
        if constexpr (
            std::numeric_limits<Count>::digits
            > std::numeric_limits<log::Timestamp>::digits) {
            const Count maximum = static_cast<Count>(
                (std::numeric_limits<log::Timestamp>::max)());
            if (count > maximum) {
                return (std::numeric_limits<log::Timestamp>::max)();
            }
        }
        return static_cast<log::Timestamp>(count);
    }
};

} // namespace platform
} // namespace cms
