#include <type_traits>
#include <utility>

#include <cms/log/clock.h>
#include <cms/platform/arduino_millis_clock.h>
#include <cms/platform/freertos_static_mutex.h>

static_assert(std::is_default_constructible<
    cms::platform::ArduinoMillisClock>::value,
    "ArduinoMillisClock must be default constructible");
static_assert(std::is_same<
    decltype(std::declval<cms::platform::ArduinoMillisClock&>()
        .nowMilliseconds()),
    cms::log::Timestamp>::value,
    "ArduinoMillisClock must return Timestamp");
static_assert(noexcept(
    std::declval<cms::platform::ArduinoMillisClock&>().nowMilliseconds()),
    "ArduinoMillisClock must preserve noexcept");
static_assert(std::is_default_constructible<
    cms::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex must be default constructible");
static_assert(!std::is_copy_constructible<
    cms::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex copy must be deleted");
static_assert(!std::is_move_constructible<
    cms::platform::FreeRtosStaticMutex>::value,
    "FreeRtosStaticMutex move must be deleted");
static_assert(noexcept(
    std::declval<cms::platform::FreeRtosStaticMutex&>().lock()),
    "FreeRtosStaticMutex lock must be noexcept");
static_assert(noexcept(
    std::declval<cms::platform::FreeRtosStaticMutex&>().unlock()),
    "FreeRtosStaticMutex unlock must be noexcept");
