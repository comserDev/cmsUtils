#pragma once

#include <cstddef>

#include <cms/status.h>
#include <cms/string_buffer.h>
#include <cms/string_view.h>

namespace cms {
namespace string {

inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

int compare(StringView lhs, StringView rhs) noexcept;
bool equals(StringView lhs, StringView rhs) noexcept;
bool startsWith(StringView value, StringView prefix) noexcept;
bool endsWith(StringView value, StringView suffix) noexcept;

std::size_t find(
    StringView value,
    StringView needle,
    std::size_t start = 0) noexcept;

std::size_t findLast(StringView value, StringView needle) noexcept;

// These four byte-copy operations support overlapping input/output storage.
WriteResult copy(StringView input, StringBuffer output) noexcept;
WriteResult copyTruncated(StringView input, StringBuffer output) noexcept;
WriteResult append(StringView input, StringBuffer output) noexcept;
WriteResult appendTruncated(StringView input, StringBuffer output) noexcept;

// All source views must remain readable during the write. Output storage must
// not overlap input, needle, or replacement storage. Failure is transactional.
WriteResult replaceAll(
    StringView input,
    StringView needle,
    StringView replacement,
    StringBuffer output) noexcept;

} // namespace string
} // namespace cms
