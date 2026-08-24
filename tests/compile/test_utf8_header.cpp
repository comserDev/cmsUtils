#include <cms/utf8.h>

void compileUtf8HeaderIndependently() {
    const cms::StringView input("text");
    const cms::utf8::DecodeResult decoded = cms::utf8::decodeNext(input, 0);
    (void)decoded;
}
