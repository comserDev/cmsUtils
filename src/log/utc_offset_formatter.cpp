#include <cms/log/utc_offset_formatter.h>

#include <cstddef>
#include <cstdint>

namespace cms {
namespace log {

namespace {

void writeTwoDigits(char* destination, int value) noexcept {
    destination[0] = static_cast<char>('0' + value / 10);
    destination[1] = static_cast<char>('0' + value % 10);
}

} // namespace

WriteResult formatUtcOffsetTimestamp(
    Timestamp unixEpochMilliseconds,
    int utcOffsetMinutes,
    StringBuffer output) noexcept {
    if (!output.valid()
        || utcOffsetMinutes < minUtcOffsetMinutes
        || utcOffsetMinutes > maxUtcOffsetMinutes) {
        return {Status::invalid_argument, 0, 0};
    }
    if (utcOffsetTimestampSize > output.maxSize()) {
        return {Status::no_space, 0, utcOffsetTimestampSize};
    }

    constexpr std::uint64_t millisecondsPerSecond = 1000;
    constexpr int secondsPerMinute = 60;
    constexpr int secondsPerHour = 60 * secondsPerMinute;
    constexpr int secondsPerDay = 24 * secondsPerHour;

    // 먼저 하루 안의 초로 줄여 offset 계산에서 uint64_t overflow를 피한다.
    const int utcSecondsOfDay = static_cast<int>(
        (unixEpochMilliseconds / millisecondsPerSecond)
        % static_cast<std::uint64_t>(secondsPerDay));
    int localSecondsOfDay =
        utcSecondsOfDay + utcOffsetMinutes * secondsPerMinute;
    localSecondsOfDay %= secondsPerDay;
    if (localSecondsOfDay < 0) {
        localSecondsOfDay += secondsPerDay;
    }

    const int hour = localSecondsOfDay / secondsPerHour;
    const int minute =
        (localSecondsOfDay % secondsPerHour) / secondsPerMinute;
    const int second = localSecondsOfDay % secondsPerMinute;

    writeTwoDigits(output.data(), hour);
    output.data()[2] = ':';
    writeTwoDigits(output.data() + 3, minute);
    output.data()[5] = ':';
    writeTwoDigits(output.data() + 6, second);

    const Status committed = output.commit(utcOffsetTimestampSize);
    if (committed != Status::ok) {
        return {committed, 0, utcOffsetTimestampSize};
    }
    return {
        Status::ok,
        utcOffsetTimestampSize,
        utcOffsetTimestampSize};
}

} // namespace log
} // namespace cms
