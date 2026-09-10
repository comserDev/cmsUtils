#pragma once

#include <cms/util/ini/document.h>

namespace cms {
namespace util {
namespace ini {

Status loadFile(
    const char* path,
    Document& document,
    TextMode mode);
Status saveFile(const char* path, const Document& document);

} // namespace ini
} // namespace util
} // namespace cms
