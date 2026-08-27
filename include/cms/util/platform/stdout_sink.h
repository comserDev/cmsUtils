#pragma once

#include <cstdio>

#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace platform {

// V1처럼 explicit flush는 하지 않고 embedded NUL까지 명시된 byte 수로 출력한다.
class StdoutSink {
public:
    Status write(StringView text) {
        if (text.empty()) {
            return Status::ok;
        }

        return std::fwrite(text.data(), 1, text.size(), stdout) == text.size()
            ? Status::ok
            : Status::io_error;
    }
};

} // namespace platform
} // namespace util
} // namespace cms
