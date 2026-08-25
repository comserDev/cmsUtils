#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

#include <cms/static_queue.h>
#include <cms/sync/lock_guard.h>
#include <cms/sync/mutex_ref.h>
#include <cms/sync/null_mutex.h>
#include <cms/synchronized_queue.h>

#include "test.h"

namespace {

struct CountingMutex {
    void lock() noexcept {
        ++locks;
        locked = true;
    }

    void unlock() noexcept {
        ++unlocks;
        locked = false;
    }

    int locks = 0;
    int unlocks = 0;
    bool locked = false;
};

void checkOperation(
    const CountingMutex& mutex,
    int previousLocks,
    int previousUnlocks) {
    CMS_TEST_CHECK(mutex.locks == previousLocks + 1);
    CMS_TEST_CHECK(mutex.unlocks == previousUnlocks + 1);
    CMS_TEST_CHECK(!mutex.locked);
}

struct MoveOnly {
    explicit MoveOnly(int initialValue) noexcept
        : value(initialValue) {}

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&& other) noexcept
        : value(other.value) {
        other.value = -1;
    }

    MoveOnly& operator=(MoveOnly&&) = delete;

    int value;
};

struct NonDefault {
    NonDefault() = delete;

    explicit NonDefault(int initialValue) noexcept
        : value(initialValue) {}

    int value;
};

struct LifetimeTracker {
    LifetimeTracker(int initialValue, int& destructorCount) noexcept
        : value(initialValue), destructors(&destructorCount) {}

    ~LifetimeTracker() noexcept {
        ++(*destructors);
    }

    int value;
    int* destructors;
};

} // namespace

int main() {
    CountingMutex guardMutex;
    {
        cms::sync::LockGuard<CountingMutex> guard(guardMutex);
        CMS_TEST_CHECK(guardMutex.locked);
        CMS_TEST_CHECK(guardMutex.locks == 1);
        CMS_TEST_CHECK(guardMutex.unlocks == 0);
    }
    CMS_TEST_CHECK(!guardMutex.locked);
    CMS_TEST_CHECK(guardMutex.locks == 1);
    CMS_TEST_CHECK(guardMutex.unlocks == 1);

    CountingMutex referencedMutex;
    cms::sync::MutexRef<CountingMutex> mutexRef(referencedMutex);
    cms::sync::MutexRef<CountingMutex> copiedRef = mutexRef;
    mutexRef.lock();
    CMS_TEST_CHECK(referencedMutex.locked);
    CMS_TEST_CHECK(referencedMutex.locks == 1);
    copiedRef.unlock();
    CMS_TEST_CHECK(!referencedMutex.locked);
    CMS_TEST_CHECK(referencedMutex.unlocks == 1);

    using ExternalQueue = cms::SynchronizedQueue<
        cms::StaticQueue<int, 3>,
        cms::sync::MutexRef<CountingMutex>>;

    CountingMutex externalMutex;
    ExternalQueue synchronized{
        cms::sync::MutexRef<CountingMutex>(externalMutex)};

    CMS_TEST_CHECK(synchronized.capacity() == 3);
    CMS_TEST_CHECK(externalMutex.locks == 0);
    CMS_TEST_CHECK(externalMutex.unlocks == 0);

    int locksBefore = externalMutex.locks;
    int unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.empty());
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.push(1) == cms::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.emplace(2) == cms::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.size() == 2);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(!synchronized.full());
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    int consumed = 0;
    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int& value) {
        CMS_TEST_CHECK(externalMutex.locked);
        CMS_TEST_CHECK(value == 1);
        consumed = value;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(consumed == 1);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.size() == 1);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int& value) {
        CMS_TEST_CHECK(externalMutex.locked);
        CMS_TEST_CHECK(value == 2);
        consumed = value;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(consumed == 2);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.push(3) == cms::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.pop() == cms::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    bool emptyConsumerCalled = false;
    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int&) {
        emptyConsumerCalled = true;
    }) == cms::Status::out_of_range);
    CMS_TEST_CHECK(!emptyConsumerCalled);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.pop() == cms::Status::out_of_range);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    CMS_TEST_CHECK(synchronized.push(10) == cms::Status::ok);
    CMS_TEST_CHECK(synchronized.push(20) == cms::Status::ok);
    CMS_TEST_CHECK(synchronized.emplace(30) == cms::Status::ok);
    CMS_TEST_CHECK(synchronized.size() == 3);
    CMS_TEST_CHECK(synchronized.full());

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.emplace(40) == cms::Status::no_space);
    checkOperation(externalMutex, locksBefore, unlocksBefore);
    CMS_TEST_CHECK(synchronized.size() == 3);
    CMS_TEST_CHECK(synchronized.full());

    int preservedFront = 0;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int& value) {
        preservedFront = value;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(preservedFront == 10);

    using NullQueue = cms::SynchronizedQueue<
        cms::StaticQueue<int, 3>,
        cms::sync::NullMutex>;

    NullQueue nullQueue;
    CMS_TEST_CHECK(nullQueue.capacity() == 3);
    CMS_TEST_CHECK(nullQueue.empty());
    CMS_TEST_CHECK(nullQueue.push(10) == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.emplace(20) == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.size() == 2);
    CMS_TEST_CHECK(!nullQueue.empty());
    CMS_TEST_CHECK(!nullQueue.full());

    int nullConsumed = 0;
    CMS_TEST_CHECK(nullQueue.consumeFront([&](int& value) {
        nullConsumed = value;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(nullConsumed == 10);
    CMS_TEST_CHECK(nullQueue.size() == 1);
    CMS_TEST_CHECK(nullQueue.pop() == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.empty());
    CMS_TEST_CHECK(nullQueue.push(30) == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.push(40) == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.push(50) == cms::Status::ok);
    CMS_TEST_CHECK(nullQueue.full());
    CMS_TEST_CHECK(nullQueue.push(60) == cms::Status::no_space);

    using MoveQueue = cms::SynchronizedQueue<
        cms::StaticQueue<MoveOnly, 2>,
        cms::sync::NullMutex>;

    MoveQueue moveQueue;
    MoveOnly moveSource(71);
    CMS_TEST_CHECK(moveQueue.push(std::move(moveSource)) == cms::Status::ok);
    CMS_TEST_CHECK(moveSource.value == -1);
    CMS_TEST_CHECK(moveQueue.emplace(72) == cms::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 71);
    }) == cms::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 72);
    }) == cms::Status::ok);
    CMS_TEST_CHECK(moveQueue.empty());

    using NonDefaultQueue = cms::SynchronizedQueue<
        cms::StaticQueue<NonDefault, 2>,
        cms::sync::NullMutex>;

    NonDefaultQueue nonDefaultQueue;
    CMS_TEST_CHECK(nonDefaultQueue.emplace(81) == cms::Status::ok);
    CMS_TEST_CHECK(nonDefaultQueue.consumeFront([](NonDefault& value) {
        CMS_TEST_CHECK(value.value == 81);
    }) == cms::Status::ok);
    CMS_TEST_CHECK(nonDefaultQueue.empty());

    int destructorCount = 0;
    using LifetimeQueue = cms::SynchronizedQueue<
        cms::StaticQueue<LifetimeTracker, 2>,
        cms::sync::NullMutex>;

    LifetimeQueue lifetimeQueue;
    CMS_TEST_CHECK(lifetimeQueue.emplace(91, destructorCount) == cms::Status::ok);
    CMS_TEST_CHECK(destructorCount == 0);
    bool consumerRan = false;
    CMS_TEST_CHECK(lifetimeQueue.consumeFront([&](LifetimeTracker& value) {
        CMS_TEST_CHECK(value.value == 91);
        CMS_TEST_CHECK(destructorCount == 0);
        consumerRan = true;
    }) == cms::Status::ok);
    CMS_TEST_CHECK(consumerRan);
    CMS_TEST_CHECK(destructorCount == 1);
    CMS_TEST_CHECK(lifetimeQueue.empty());

    std::printf(
        "sizeof(cms::StaticQueue<std::uint8_t, 8>)=%zu\n",
        sizeof(cms::StaticQueue<std::uint8_t, 8>));
    std::printf(
        "sizeof(cms::SynchronizedQueue<StaticQueue<uint8_t, 8>, "
        "NullMutex>)=%zu\n",
        sizeof(cms::SynchronizedQueue<
            cms::StaticQueue<std::uint8_t, 8>,
            cms::sync::NullMutex>));
    std::printf(
        "sizeof(cms::sync::MutexRef<CountingMutex>)=%zu\n",
        sizeof(cms::sync::MutexRef<CountingMutex>));

    return cms::test::finish();
}
