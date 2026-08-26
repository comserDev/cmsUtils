#include <cms/util/platform/freertos_static_mutex.h>

static_assert(
    sizeof(cms::util::platform::FreeRtosStaticMutex) > 0,
    "freertos_static_mutex.h must compile with FreeRTOS API");
