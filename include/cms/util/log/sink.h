#pragma once

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

// Sink backend는 Status write(StringView)를 제공한다. text storage는 호출 중에만
// 유효하며 backend exception은 logger가 변환하거나 숨기지 않는다.

} // namespace log
} // namespace util
} // namespace cms
