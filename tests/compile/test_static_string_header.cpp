#include <cms/util/static_string.h>

constexpr cms::util::StaticString<1> standaloneEmpty;

static_assert(standaloneEmpty.empty(), "StaticString must default to empty");
static_assert(standaloneEmpty.capacity() == 1, "capacity includes the NUL byte");
static_assert(standaloneEmpty.maxSize() == 0, "StorageBytes == 1 stores no payload");
