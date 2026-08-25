#pragma once

#include <cstdint>

namespace cms {
namespace log {

using Timestamp = std::uint64_t;

// timestamp 단위는 milliseconds이며 epoch은 backend가 정한다.
// concurrent producer가 쓰는 Clock의 동시 호출 안전성도 backend 책임이다.

} // namespace log
} // namespace cms
