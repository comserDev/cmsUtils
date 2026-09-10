#pragma once

#include <cms/util/byte_view.h>

namespace cms {
namespace util {
namespace crypto {

// 같은 길이의 두 byte 영역을 모든 위치까지 확인하여 비교한다. 값이 다른 위치에서
// 조기에 종료하지 않으므로 digest나 MAC 비교에 적합하다. 다만 표준 C++만으로 모든
// compiler와 CPU에서 절대적으로 동일한 실행 시간을 보장하지는 않는다.
bool constantTimeEqual(
    ByteView lhs,
    ByteView rhs) noexcept;

} // namespace crypto
} // namespace util
} // namespace cms
