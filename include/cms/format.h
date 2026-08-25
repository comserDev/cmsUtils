#pragma once

#include <cstdint>

#include <cms/status.h>
#include <cms/string_buffer.h>

namespace cms {
namespace format {

// 정수 formatting은 base 10과 16만 지원하며 다른 base는 invalid_argument다.
// 결과에는 0x 같은 numeric prefix를 붙이지 않는다. base 16에서 uppercase는
// A-F와 a-f 중 어느 문자를 쓸지 정한다. signed 음수는 sign + magnitude로
// 표현한다. 기본 함수는 output을 교체하고 append variant는 기존 payload 뒤에
// 추가한다. 모든 연산은 실패 시 output을 바꾸지 않으며 WriteResult의 byte 수는
// terminating NUL을 포함하지 않는다.
WriteResult unsignedInteger(
    std::uint64_t value,
    StringBuffer output,
    unsigned int base = 10,
    bool uppercase = false) noexcept;

WriteResult signedInteger(
    std::int64_t value,
    StringBuffer output,
    unsigned int base = 10,
    bool uppercase = false) noexcept;

WriteResult appendUnsignedInteger(
    std::uint64_t value,
    StringBuffer output,
    unsigned int base = 10,
    bool uppercase = false) noexcept;

WriteResult appendSignedInteger(
    std::int64_t value,
    StringBuffer output,
    unsigned int base = 10,
    bool uppercase = false) noexcept;

} // namespace format
} // namespace cms
