#include <cms/util/string_buffer.h>

void compileStringBufferHeaderIndependently() {
    char storage[2] = "";
    std::size_t size = 0;
    cms::util::StringBuffer buffer(storage, sizeof(storage), size);
    (void)buffer.commit(0);
    (void)buffer;
}
