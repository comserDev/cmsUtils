#pragma once

#include <cms/util/ini/document.h>

namespace cms {
namespace util {
namespace ini {

// Host 파일을 읽어 document에 반영한다. 실패 시 기존 document를 보존한다.
Status loadFile(
    const char* path,
    Document& document,
    TextMode mode);
// document를 Host 파일에 저장한다.
Status saveFile(const char* path, const Document& document);

} // namespace ini
} // namespace util
} // namespace cms
