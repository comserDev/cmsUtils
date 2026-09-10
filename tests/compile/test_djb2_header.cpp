#include <cstdint>

#include <cms/util/hash/djb2.h>

constexpr std::uint32_t compileTimeDjb2() {
    constexpr std::uint8_t bytes[] = {0x00, 0xFF, 0x80, 0x01};
    return cms::util::hash::djb2(cms::util::ByteView(bytes));
}

static_assert(
    cms::util::hash::Djb2InitialValue == 5381U,
    "Djb2 initial value changed");
static_assert(
    cms::util::hash::djb2(cms::util::StringView("abc")) == 0x0B885C8BU,
    "Djb2 StringView constexpr contract changed");
static_assert(
    compileTimeDjb2() == 0x7C615CC5U,
    "Djb2 ByteView constexpr contract changed");

int djb2HeaderCompile() {
    cms::util::hash::Djb2 hash;
    hash.updateByte(static_cast<std::uint8_t>('a'));
    return hash.value() == 0x0002B606U ? 0 : 1;
}
