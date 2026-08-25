#pragma once

#include <cstdint>

using BaseType_t = int;
using TickType_t = std::uint32_t;

struct StaticSemaphore_t {
    std::uint32_t marker;
};

#define pdFALSE 0
#define pdTRUE 1
#define portMAX_DELAY static_cast<TickType_t>(0xFFFFFFFFu)
