#pragma once

#include <FS.h>

#include <cms/util/ini/document.h>

namespace cms {
namespace util {
namespace ini {

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
