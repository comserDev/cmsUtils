#include <cms/util/crypto/constant_time.h>

int constantTimeHeaderCompile() {
    return cms::util::crypto::constantTimeEqual(
        cms::util::ByteView(),
        cms::util::ByteView()) ? 0 : 1;
}
