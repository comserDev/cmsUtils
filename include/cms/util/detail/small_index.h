#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cms {
namespace util {
namespace detail {

// MaxValue를 표현할 수 있는 가장 작은 고정 폭 unsigned 타입을 고른다.
// uint32_t 범위를 넘는 경우에는 플랫폼의 std::size_t를 사용한다.
template<std::size_t MaxValue>
using small_index_t = typename std::conditional<
    MaxValue <= (std::numeric_limits<std::uint8_t>::max)(),
    std::uint8_t,
    typename std::conditional<
        MaxValue <= (std::numeric_limits<std::uint16_t>::max)(),
        std::uint16_t,
        typename std::conditional<
            MaxValue <= (std::numeric_limits<std::uint32_t>::max)(),
            std::uint32_t,
            std::size_t>::type>::type>::type;

} // namespace detail
} // namespace util
} // namespace cms
