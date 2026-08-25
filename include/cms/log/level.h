#pragma once

#include <cstdint>

namespace cms {
namespace log {

enum class Level : std::uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
    critical
};

} // namespace log
} // namespace cms
