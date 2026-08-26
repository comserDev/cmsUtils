#include <chrono>
#include <cstddef>
#include <cstdio>
#include <limits>

#include <cms/log/clock.h>
#include <cms/platform/system_clock.h>

#include "test.h"

namespace {

cms::log::Timestamp systemClockSnapshot() noexcept {
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
        > std::numeric_limits<cms::log::Timestamp>::digits) {
        const Count maximum = static_cast<Count>(
            (std::numeric_limits<cms::log::Timestamp>::max)());
        if (count > maximum) {
            return (std::numeric_limits<cms::log::Timestamp>::max)();
        }
    }
    return static_cast<cms::log::Timestamp>(count);
}

void checkCurrentSystemClock(cms::platform::SystemClock& clock) {
    const cms::log::Timestamp before = systemClockSnapshot();
    const cms::log::Timestamp actual = clock.nowMilliseconds();
    const cms::log::Timestamp after = systemClockSnapshot();
    const cms::log::Timestamp lower = before < after ? before : after;
    const cms::log::Timestamp upper = before < after ? after : before;

    // backward adjustment를 허용하면서 같은 epoch와 millisecond scale인지 확인한다.
    CMS_TEST_CHECK(actual >= lower);
    CMS_TEST_CHECK(actual <= upper);
}

} // namespace

int main() {
    cms::platform::SystemClock clock;
    checkCurrentSystemClock(clock);
    checkCurrentSystemClock(clock);

    std::printf(
        "sizeof(cms::platform::SystemClock)=%zu\n",
        sizeof(cms::platform::SystemClock));

    return cms::test::finish();
}
