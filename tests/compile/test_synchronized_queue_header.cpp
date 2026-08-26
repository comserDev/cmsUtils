#include <cms/util/sync/synchronized_queue.h>

struct HeaderQueue {
    std::size_t size() const noexcept { return 0; }
    std::size_t capacity() const noexcept { return 1; }
    bool empty() const noexcept { return true; }
    bool full() const noexcept { return false; }
};

struct HeaderMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

using HeaderSynchronizedQueue =
    cms::util::sync::SynchronizedQueue<HeaderQueue, HeaderMutex>;

static_assert(
    sizeof(HeaderSynchronizedQueue) > 0,
    "synchronized_queue.h must compile independently");
