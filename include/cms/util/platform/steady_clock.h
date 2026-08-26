#pragma once

#include <chrono>

#include <cms/util/log/clock.h>

namespace cms {
namespace util {
namespace platform {

// wall clock이 아닌 monotonic relative timestamp를 milliseconds로 반환한다.
class SteadyClock {
public:
    log::Timestamp nowMilliseconds() noexcept {
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
        const auto count = elapsed.count();
        return count > 0 ? static_cast<log::Timestamp>(count) : 0;
    }
};

} // namespace platform
} // namespace util
} // namespace cms
