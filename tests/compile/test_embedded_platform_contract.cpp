#include <type_traits>
#include <utility>

#include <cms/util/log/clock.h>
#include <cms/util/platform/arduino_millis_clock.h>
#include <cms/util/platform/freertos_static_mutex.h>

static_assert(std::is_default_constructible<
    cms::util::platform::ArduinoMillisClock>::value,
    "ArduinoMillisClock must be default constructible");
static_assert(std::is_same<
    decltype(std::declval<cms::util::platform::ArduinoMillisClock&>()
        .nowMilliseconds()),
    cms::util::log::Timestamp>::value,
    "ArduinoMillisClock must return Timestamp");
static_assert(noexcept(
    std::declval<cms::util::platform::ArduinoMillisClock&>().nowMilliseconds()),
    "ArduinoMillisClock must preserve noexcept");
static_assert(std::is_default_constructible<
    cms::util::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex must be default constructible");
static_assert(!std::is_copy_constructible<
    cms::util::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex copy must be deleted");
static_assert(!std::is_move_constructible<
    cms::util::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex move must be deleted");
static_assert(noexcept(
    std::declval<cms::util::platform::FreeRtosStaticMutex&>().lock()),
    "FreeRtosStaticMutex lock must be noexcept");
static_assert(noexcept(
    std::declval<cms::util::platform::FreeRtosStaticMutex&>().unlock()),
    "FreeRtosStaticMutex unlock must be noexcept");
