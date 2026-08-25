#include <type_traits>
#include <utility>

#include <cms/sync/lock_guard.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>

namespace {

struct NothrowMutex {
    void lock() noexcept {}
    void unlock() noexcept {}
};

struct ThrowingMutex {
    void lock() noexcept(false) {}
    void unlock() noexcept(false) {}
};

using Guard = cms::sync::LockGuard<NothrowMutex>;
using Ref = cms::sync::MutexRef<NothrowMutex>;

} // namespace

static_assert(std::is_constructible<Guard, NothrowMutex&>::value,
    "LockGuard must bind a mutex reference");
static_assert(!std::is_copy_constructible<Guard>::value,
    "LockGuard copy must be deleted");
static_assert(!std::is_copy_assignable<Guard>::value,
    "LockGuard copy assignment must be deleted");
static_assert(!std::is_move_constructible<Guard>::value,
    "LockGuard move must be deleted");
static_assert(!std::is_move_assignable<Guard>::value,
    "LockGuard move assignment must be deleted");
static_assert(std::is_nothrow_constructible<Guard, NothrowMutex&>::value,
    "LockGuard must forward nothrow lock");
static_assert(std::is_nothrow_destructible<Guard>::value,
    "LockGuard must forward nothrow unlock");
static_assert(!std::is_nothrow_constructible<
    cms::sync::LockGuard<ThrowingMutex>, ThrowingMutex&>::value,
    "LockGuard must preserve a throwing lock");
static_assert(!std::is_nothrow_destructible<
    cms::sync::LockGuard<ThrowingMutex>>::value,
    "LockGuard must preserve a throwing unlock");

static_assert(std::is_empty<cms::sync::NullMutex>::value,
    "NullMutex must remain empty");
static_assert(noexcept(std::declval<cms::sync::NullMutex&>().lock()),
    "NullMutex lock must be noexcept");
static_assert(noexcept(std::declval<cms::sync::NullMutex&>().unlock()),
    "NullMutex unlock must be noexcept");

static_assert(!std::is_default_constructible<Ref>::value,
    "MutexRef must not have a null state");
static_assert(std::is_constructible<Ref, NothrowMutex&>::value,
    "MutexRef must bind a mutex reference");
static_assert(std::is_copy_constructible<Ref>::value,
    "MutexRef copies must alias the same mutex");
static_assert(std::is_copy_assignable<Ref>::value,
    "MutexRef copy assignment must preserve alias semantics");
static_assert(noexcept(std::declval<Ref&>().lock()),
    "MutexRef must forward nothrow lock");
static_assert(noexcept(std::declval<Ref&>().unlock()),
    "MutexRef must forward nothrow unlock");
static_assert(!noexcept(
    std::declval<cms::sync::MutexRef<ThrowingMutex>&>().lock()),
    "MutexRef must preserve a throwing lock");
static_assert(!noexcept(
    std::declval<cms::sync::MutexRef<ThrowingMutex>&>().unlock()),
    "MutexRef must preserve a throwing unlock");
