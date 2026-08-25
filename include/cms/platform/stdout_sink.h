#pragma once

#include <cstdio>

#include <cms/string_view.h>

namespace cms {
namespace platform {

// V1처럼 explicit flush는 하지 않고 embedded NUL까지 명시된 byte 수로 출력한다.
class StdoutSink {
public:
    void write(StringView text) {
        if (text.empty()) {
            return;
        }

        (void)std::fwrite(text.data(), 1, text.size(), stdout);
    }
};

} // namespace platform
} // namespace cms
