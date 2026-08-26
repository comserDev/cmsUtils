#pragma once

#include <cstddef>

#include <cms/util/status.h>
#include <cms/util/string_buffer.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace utf8 {

struct DecodeResult {
    Status status;
    char32_t codePoint;
    // bytes는 이번 decode가 소비한 input byte 수다.
    std::size_t bytes;
};

// 올바른 Unicode scalar value 하나를 decode한다. 잘못된 UTF-8은 byte 하나를
// 소비하고 U+FFFD를 반환한다. 끝 이상의 offset은 input을 소비하지 않고
// out_of_range를 반환한다.
DecodeResult decodeNext(StringView input, std::size_t offset) noexcept;

// input 전체가 올바른 UTF-8인지 검사한다.
Status validate(StringView input) noexcept;

// grapheme cluster가 아니라 Unicode scalar value 수를 센다. 실패하면 value에는
// 오류 전까지 센 개수, consumed에는 잘못된 byte의 offset이 들어간다.
ParseResult<std::size_t> count(StringView input) noexcept;

// code point index 기준 substring으로 output을 교체한다. 요청한 count가 남은
// code point보다 크면 입력 끝까지만 선택한다. input과 output storage는 겹치면
// 안 되며 실패 시 output은 바뀌지 않는다.
WriteResult substring(
    StringView input,
    std::size_t firstCodePoint,
    std::size_t count,
    StringBuffer output) noexcept;

// 잘못된 input byte마다 UTF-8 U+FFFD를 하나씩 넣어 output을 교체한다. input과
// output storage는 겹치면 안 되며 실패 시 output은 바뀌지 않는다.
WriteResult sanitize(StringView input, StringBuffer output) noexcept;

} // namespace utf8
} // namespace util
} // namespace cms
