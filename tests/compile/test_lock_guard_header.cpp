#include <cms/util/sync/lock_guard.h>

struct HeaderMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

static_assert(
    sizeof(cms::util::sync::LockGuard<HeaderMutex>) > 0,
    "lock_guard.h must compile independently");
