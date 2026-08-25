#pragma once

#include <FreeRTOS.h>

using SemaphoreHandle_t = StaticSemaphore_t*;

namespace cms_test_freertos {

inline int createCalls = 0;
inline int takeCalls = 0;
inline int giveCalls = 0;
inline int deleteCalls = 0;
inline StaticSemaphore_t* createdStorage = nullptr;
inline SemaphoreHandle_t takenHandle = nullptr;
inline SemaphoreHandle_t givenHandle = nullptr;
inline SemaphoreHandle_t deletedHandle = nullptr;
inline TickType_t takeTicks = 0;
inline int takeFailuresBeforeSuccess = 0;

inline void reset() noexcept {
    createCalls = 0;
    takeCalls = 0;
    giveCalls = 0;
    deleteCalls = 0;
    createdStorage = nullptr;
    takenHandle = nullptr;
    givenHandle = nullptr;
    deletedHandle = nullptr;
    takeTicks = 0;
    takeFailuresBeforeSuccess = 0;
}

} // namespace cms_test_freertos

inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(
    StaticSemaphore_t* storage) noexcept {
    ++cms_test_freertos::createCalls;
    cms_test_freertos::createdStorage = storage;
    return storage;
}

inline BaseType_t xSemaphoreTake(
    SemaphoreHandle_t handle,
    TickType_t ticks) noexcept {
    ++cms_test_freertos::takeCalls;
    cms_test_freertos::takenHandle = handle;
    cms_test_freertos::takeTicks = ticks;
    if (cms_test_freertos::takeFailuresBeforeSuccess > 0) {
        --cms_test_freertos::takeFailuresBeforeSuccess;
        return pdFALSE;
    }
    return pdTRUE;
}

inline BaseType_t xSemaphoreGive(SemaphoreHandle_t handle) noexcept {
    ++cms_test_freertos::giveCalls;
    cms_test_freertos::givenHandle = handle;
    return pdTRUE;
}

inline void vSemaphoreDelete(SemaphoreHandle_t handle) noexcept {
    ++cms_test_freertos::deleteCalls;
    cms_test_freertos::deletedHandle = handle;
}
