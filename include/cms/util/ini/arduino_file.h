#pragma once

#include <FS.h>

#include <cms/util/ini/document.h>

namespace cms {
namespace util {
namespace ini {

// fs::FS에서 파일을 읽어 document에 반영한다. 파일 읽기나 UTF-8 검증에
// 실패하면 기존 document는 변경하지 않는다.
inline Status loadFile(
    fs::FS& filesystem,
    const char* path,
    Document& document,
    TextMode mode) {
    if (path == nullptr || path[0] == '\0') {
        return Status::invalid_argument;
    }

    fs::File input = filesystem.open(path, FILE_READ);
    if (!input) return Status::io_error;

    std::string content;
    const std::size_t size = input.size();
    content.reserve(size);
    char buffer[128];
    while (input.available() > 0) {
        const std::size_t available =
            static_cast<std::size_t>(input.available());
        const std::size_t requested = available < sizeof(buffer)
            ? available
            : sizeof(buffer);
        const std::size_t read = input.readBytes(buffer, requested);
        if (read == 0) {
            input.close();
            return Status::io_error;
        }
        content.append(buffer, read);
    }
    input.close();
    return document.replaceFromText(content, mode);
}

// document를 fs::FS 파일에 저장한다. 출력 write가 실패하면 io_error를 반환한다.
inline Status saveFile(
    fs::FS& filesystem,
    const char* path,
    const Document& document) {
    if (path == nullptr || path[0] == '\0') {
        return Status::invalid_argument;
    }

    fs::File output = filesystem.open(path, FILE_WRITE);
    if (!output) return Status::io_error;

    const std::string content = document.serialize();
    if (!content.empty() &&
        output.write(
            reinterpret_cast<const std::uint8_t*>(content.data()),
            content.size()) != content.size()) {
        output.close();
        return Status::io_error;
    }
    output.flush();
    output.close();
    return Status::ok;
}

} // namespace ini
} // namespace util
} // namespace cms
