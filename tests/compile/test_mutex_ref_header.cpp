#include <cms/sync/mutex_ref.h>

struct HeaderMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

static_assert(
    sizeof(cms::sync::MutexRef<HeaderMutex>) > 0,
    "mutex_ref.h must compile independently");
