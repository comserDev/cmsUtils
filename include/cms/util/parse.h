#pragma once

#include <cstdint>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace parse {

// 정수 parsing은 base 10과 16만 지원하며 byte 0부터 whitespace를 건너뛰지 않고
// 시작한다. 성공은 하나의 numeric prefix만 소비하므로 전체 입력을 소비할 필요가
// 없고, trailing non-digit을 만나면 그 앞까지 성공으로 처리한다. unsigned 입력은
// sign을 허용하지 않고 signed 입력은 맨 앞의 + 또는 - 하나를 허용한다. base 16은
// optional sign 뒤에 0x나 0X를 허용하며 prefix 뒤에는 hex digit이 하나 이상 있어야
// 한다. overflow는 out_of_range, value 0, offending digit의 zero-based byte offset을
// 반환한다. invalid_argument는 value와 consumed가 모두 0이다. StringView의 명시적
// 길이를 따르므로 embedded NUL도 일반 non-digit byte로 처리한다.
ParseResult<std::uint64_t> unsignedInteger(
    StringView input,
    unsigned int base = 10) noexcept;

ParseResult<std::int64_t> signedInteger(
    StringView input,
    unsigned int base = 10) noexcept;

} // namespace parse
} // namespace util
} // namespace cms
