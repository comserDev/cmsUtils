#pragma once

#include <cstdio>
#include <utility>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace platform {

enum class FileOpenMode {
    append,
    truncate
};

// Standard C stdio를 소유하는 host adapter다. write 성공은 stream이 bytes를
// 수용했다는 뜻이며 fsync 수준의 durability를 보장하지 않는다.
class StdFileSink {
public:
    StdFileSink() noexcept = default;

    ~StdFileSink() noexcept {
        if (file_ != nullptr) {
            (void)std::fclose(file_);
        }
    }

    StdFileSink(const StdFileSink&) = delete;
    StdFileSink& operator=(const StdFileSink&) = delete;

    StdFileSink(StdFileSink&& other) noexcept
        : file_(other.file_) {
        other.file_ = nullptr;
    }

    StdFileSink& operator=(StdFileSink&&) = delete;

    // 열린 handle의 implicit close/reopen은 하지 않고 caller의 상태 오류로 처리한다.
    Status open(
        const char* path,
        FileOpenMode mode = FileOpenMode::append) {
        if (path == nullptr || path[0] == '\0' || file_ != nullptr) {
            return Status::invalid_argument;
        }

        const char* modeText = nullptr;
        switch (mode) {
        case FileOpenMode::append:
            modeText = "ab";
            break;
        case FileOpenMode::truncate:
            modeText = "wb";
            break;
        default:
            return Status::invalid_argument;
        }
#if defined(_MSC_VER)
        std::FILE* opened = nullptr;
        if (::fopen_s(&opened, path, modeText) != 0) {
            opened = nullptr;
        }
        file_ = opened;
#else
        file_ = std::fopen(path, modeText);
#endif
        return file_ != nullptr ? Status::ok : Status::io_error;
    }

    Status write(StringView data) {
        if (file_ == nullptr) {
            return Status::invalid_argument;
        }
        if (data.empty()) {
            return Status::ok;
        }

        return std::fwrite(data.data(), 1, data.size(), file_) == data.size()
            ? Status::ok
            : Status::io_error;
    }

    Status flush() {
        if (file_ == nullptr) {
            return Status::invalid_argument;
        }
        return std::fflush(file_) == 0 ? Status::ok : Status::io_error;
    }

    Status close() {
        if (file_ == nullptr) {
            return Status::ok;
        }

        std::FILE* const closing = file_;
        file_ = nullptr;
        return std::fclose(closing) == 0 ? Status::ok : Status::io_error;
    }

    bool isOpen() const noexcept {
        return file_ != nullptr;
    }

private:
    std::FILE* file_ = nullptr;
};

} // namespace platform
} // namespace util
} // namespace cms
