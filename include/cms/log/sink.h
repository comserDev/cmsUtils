#pragma once

#include <cms/string_view.h>

namespace cms {
namespace log {

// Sink backend는 void write(StringView)를 제공한다. text storage는 호출 중에만
// 유효하므로 이후에 필요하면 backend가 직접 복사해야 한다.

} // namespace log
} // namespace cms
