#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/static_queue.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>
#include <cms/synchronized_queue.h>

namespace {

struct Element {
    explicit Element(int initialValue) noexcept
        : value(initialValue) {}

    Element(const Element&) noexcept = default;
    Element(Element&&) noexcept = default;
    ~Element() noexcept = default;

    int value;
};

struct Consumer {
    void operator()(Element&) const noexcept {}
};

struct NothrowMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

struct ThrowingMutex {
    void lock() noexcept(false) {}
    void unlock() noexcept(false) {}
};

struct NonMovableMutex {
    NonMovableMutex() noexcept = default;
    NonMovableMutex(const NonMovableMutex&) = delete;
    NonMovableMutex& operator=(const NonMovableMutex&) = delete;
    NonMovableMutex(NonMovableMutex&&) = delete;
    NonMovableMutex& operator=(NonMovableMutex&&) = delete;

    void lock() noexcept {}
    void unlock() noexcept {}
};

template<class Type, class = void>
struct HasQueueAccessor : std::false_type {};

template<class Type>
struct HasQueueAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().queue())>> : std::true_type {};

template<class Type, class = void>
struct HasMutexAccessor : std::false_type {};

template<class Type>
struct HasMutexAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().mutex())>> : std::true_type {};

template<class Type, class = void>
struct HasFrontAccessor : std::false_type {};

template<class Type>
struct HasFrontAccessor<
    Type,
    std::void_t<decltype(std::declval<Type&>().front())>> : std::true_type {};

using Queue = cms::StaticQueue<Element, 4>;
using SyncQueue = cms::SynchronizedQueue<Queue, cms::sync::NullMutex>;
using Ref = cms::sync::MutexRef<NothrowMutex>;
using RefQueue = cms::SynchronizedQueue<Queue, Ref>;
using ThrowingQueue = cms::SynchronizedQueue<Queue, ThrowingMutex>;
using NonMovableMutexQueue =
    cms::SynchronizedQueue<Queue, NonMovableMutex>;

} // namespace

static_assert(std::is_nothrow_default_constructible<SyncQueue>::value,
    "SynchronizedQueue default construction must forward noexcept");
static_assert(std::is_default_constructible<NonMovableMutexQueue>::value,
    "SynchronizedQueue must own a non-movable default mutex");
static_assert(
    std::is_nothrow_default_constructible<NonMovableMutexQueue>::value,
    "Non-movable mutex default construction must remain noexcept");
static_assert(!std::is_copy_constructible<SyncQueue>::value,
    "SynchronizedQueue copy must be deleted");
static_assert(!std::is_copy_assignable<SyncQueue>::value,
    "SynchronizedQueue copy assignment must be deleted");
static_assert(!std::is_move_constructible<SyncQueue>::value,
    "SynchronizedQueue move must be deleted");
static_assert(!std::is_move_assignable<SyncQueue>::value,
    "SynchronizedQueue move assignment must be deleted");

static_assert(!std::is_default_constructible<RefQueue>::value,
    "MutexRef-backed queue requires an external mutex");
static_assert(std::is_constructible<RefQueue, Ref>::value,
    "MutexRef-backed queue must accept its mutex backend");

static_assert(std::is_same<
    decltype(std::declval<const SyncQueue&>().size()),
    std::size_t>::value, "size has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const SyncQueue&>().capacity()),
    std::size_t>::value, "capacity has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const SyncQueue&>().empty()),
    bool>::value, "empty has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<const SyncQueue&>().full()),
    bool>::value, "full has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().pop()),
    cms::Status>::value, "pop has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().emplace(1)),
    cms::Status>::value, "emplace has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().push(
        std::declval<const Element&>())),
    cms::Status>::value, "copy push has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().push(
        std::declval<Element&&>())),
    cms::Status>::value, "move push has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().pushOverwrite(
        std::declval<const Element&>())),
    cms::Status>::value, "copy overwrite push has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().pushOverwrite(
        std::declval<Element&&>())),
    cms::Status>::value, "move overwrite push has the wrong return type");
static_assert(std::is_same<
    decltype(std::declval<SyncQueue&>().consumeFront(Consumer{})),
    cms::Status>::value, "consumeFront has the wrong return type");

static_assert(noexcept(std::declval<const SyncQueue&>().size()),
    "size must forward noexcept");
static_assert(noexcept(std::declval<const SyncQueue&>().capacity()),
    "capacity must forward noexcept");
static_assert(noexcept(std::declval<const SyncQueue&>().empty()),
    "empty must forward noexcept");
static_assert(noexcept(std::declval<const SyncQueue&>().full()),
    "full must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().pop()),
    "pop must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().emplace(1)),
    "emplace must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().push(
    std::declval<const Element&>())), "copy push must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().push(
    std::declval<Element&&>())), "move push must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().pushOverwrite(
    std::declval<const Element&>())),
    "copy overwrite push must forward noexcept");
static_assert(noexcept(std::declval<SyncQueue&>().pushOverwrite(
    std::declval<Element&&>())),
    "move overwrite push must forward noexcept");
static_assert(!noexcept(std::declval<const ThrowingQueue&>().size()),
    "throwing lock must not become noexcept");
static_assert(!noexcept(std::declval<ThrowingQueue&>().pop()),
    "throwing unlock must not become noexcept");

static_assert(!HasQueueAccessor<SyncQueue>::value,
    "raw Queue access must not escape the lock scope");
static_assert(!HasMutexAccessor<SyncQueue>::value,
    "raw Mutex access must not escape the wrapper");
static_assert(!HasFrontAccessor<SyncQueue>::value,
    "raw front pointers must not escape the lock scope");
