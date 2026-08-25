#pragma once

#include <cstddef>

#include <cms/status.h>
#include <cms/string_buffer.h>
#include <cms/string_view.h>

namespace cms {
namespace string {

inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

// UTF-8이나 locale을 해석하지 않고 unsigned byte 순서로 비교한다.
// compare는 lhs가 작거나 같거나 큰 경우 각각 -1, 0, 1을 반환한다.
int compare(StringView lhs, StringView rhs) noexcept;
bool equals(StringView lhs, StringView rhs) noexcept;
bool startsWith(StringView value, StringView prefix) noexcept;
bool endsWith(StringView value, StringView suffix) noexcept;

// find의 start는 byte index다. 빈 needle은 유효한 start를 그대로 반환하며,
// 찾지 못했거나 start가 범위를 벗어나면 npos를 반환한다.
std::size_t find(
    StringView value,
    StringView needle,
    std::size_t start = 0) noexcept;

// 마지막 byte match를 찾는다. 빈 needle은 value.size()에서 일치한다.
std::size_t findLast(StringView value, StringView needle) noexcept;

// 네 byte-copy 연산은 input과 output storage가 겹치는 경우도 지원한다.
// copy와 append는 transactional이고, Truncated variant만 부분 기록을 허용한다.
WriteResult copy(StringView input, StringBuffer output) noexcept;
WriteResult copyTruncated(StringView input, StringBuffer output) noexcept;
WriteResult append(StringView input, StringBuffer output) noexcept;
WriteResult appendTruncated(StringView input, StringBuffer output) noexcept;

// 겹치지 않는 match를 왼쪽부터 치환한다. 기록 중 모든 source view를 계속 읽을
// 수 있어야 하므로 output storage는 input, needle, replacement와 겹치면 안 된다.
// 실패 시 output은 바뀌지 않는다.
WriteResult replaceAll(
    StringView input,
    StringView needle,
    StringView replacement,
    StringBuffer output) noexcept;

} // namespace string
} // namespace cms
