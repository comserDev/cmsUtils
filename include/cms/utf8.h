#pragma once

#include <cstddef>

#include <cms/status.h>
#include <cms/string_buffer.h>
#include <cms/string_view.h>

namespace cms {
namespace utf8 {

struct DecodeResult {
    Status status;
    char32_t codePoint;
    std::size_t bytes;
};

// Invalid UTF-8 consumes one byte and produces U+FFFD. An offset at or past
// the end returns out_of_range without consuming input.
DecodeResult decodeNext(StringView input, std::size_t offset) noexcept;

Status validate(StringView input) noexcept;

// Counts Unicode scalar values, not grapheme clusters.
ParseResult<std::size_t> count(StringView input) noexcept;

// Replaces output with a code-point-indexed substring. Input and output
// storage must not overlap. Failure is transactional.
WriteResult substring(
    StringView input,
    std::size_t firstCodePoint,
    std::size_t count,
    StringBuffer output) noexcept;

// Replaces each malformed input byte with UTF-8 U+FFFD. Input and output
// storage must not overlap. Failure is transactional.
WriteResult sanitize(StringView input, StringBuffer output) noexcept;

} // namespace utf8
} // namespace cms
