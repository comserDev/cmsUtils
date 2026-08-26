#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

namespace cms {
namespace util {
namespace platform {

// 일반 task context의 blocking mutex다. ISR에서는 사용하지 않는다.
class FreeRtosStaticMutex {
public:
    FreeRtosStaticMutex() noexcept
        : storage_{}, handle_(xSemaphoreCreateMutexStatic(&storage_)) {
        if (handle_ == nullptr) {
            failStop();
        }
    }

    ~FreeRtosStaticMutex() noexcept {
        if (handle_ != nullptr) {
            vSemaphoreDelete(handle_);
        }
    }

    FreeRtosStaticMutex(const FreeRtosStaticMutex&) = delete;
    FreeRtosStaticMutex& operator=(const FreeRtosStaticMutex&) = delete;
    FreeRtosStaticMutex(FreeRtosStaticMutex&&) = delete;
    FreeRtosStaticMutex& operator=(FreeRtosStaticMutex&&) = delete;

    void lock() noexcept {
        // 정상 return했다면 실제 mutex ownership을 가진다는 contract를 지킨다.
        while (xSemaphoreTake(handle_, portMAX_DELAY) != pdTRUE) {
        }
    }

    void unlock() noexcept {
        (void)xSemaphoreGive(handle_);
    }

private:
    // Static creation failure는 synchronization-disabled 상태로 진행하지 않는다.
    static void failStop() noexcept {
        for (;;) {
        }
    }

    StaticSemaphore_t storage_;
    SemaphoreHandle_t handle_;
};

} // namespace platform
} // namespace util
} // namespace cms
