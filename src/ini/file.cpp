#include <cms/util/ini/file.h>

#if !defined(ARDUINO)

#include <fstream>
#include <iterator>

namespace cms {
namespace util {
namespace ini {

Status loadFile(
    const char* path,
    Document& document,
    TextMode mode) {
    if (path == nullptr || path[0] == '\0') {
        return Status::invalid_argument;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) return Status::io_error;

    const std::string content{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) return Status::io_error;
    return document.replaceFromText(content, mode);
}

Status saveFile(const char* path, const Document& document) {
    if (path == nullptr || path[0] == '\0') {
        return Status::invalid_argument;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return Status::io_error;

    const std::string content = document.serialize();
    if (!content.empty()) {
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
    if (!output) return Status::io_error;
    output.close();
    return output ? Status::ok : Status::io_error;
}

} // namespace ini
} // namespace util
} // namespace cms

#endif
