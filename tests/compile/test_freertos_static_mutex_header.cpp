#include <cms/platform/freertos_static_mutex.h>

static_assert(
    sizeof(cms::platform::FreeRtosStaticMutex) > 0,
    "freertos_static_mutex.h must compile with FreeRTOS API");
