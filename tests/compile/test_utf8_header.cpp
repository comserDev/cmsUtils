#include <cms/util/utf8.h>

void compileUtf8HeaderIndependently() {
    const cms::util::StringView input("text");
    const cms::util::utf8::DecodeResult decoded = cms::util::utf8::decodeNext(input, 0);
    (void)decoded;
}
