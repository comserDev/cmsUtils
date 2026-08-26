#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

#include <cms/util/static_queue.h>
#include <cms/util/sync/lock_guard.h>
#include <cms/util/sync/mutex_ref.h>
#include <cms/util/sync/null_mutex.h>
#include <cms/util/sync/synchronized_queue.h>

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
        cms::util::sync::LockGuard<CountingMutex> guard(guardMutex);
        CMS_TEST_CHECK(guardMutex.locked);
        CMS_TEST_CHECK(guardMutex.locks == 1);
        CMS_TEST_CHECK(guardMutex.unlocks == 0);
    }
    CMS_TEST_CHECK(!guardMutex.locked);
    CMS_TEST_CHECK(guardMutex.locks == 1);
    CMS_TEST_CHECK(guardMutex.unlocks == 1);

    CountingMutex referencedMutex;
    cms::util::sync::MutexRef<CountingMutex> mutexRef(referencedMutex);
    cms::util::sync::MutexRef<CountingMutex> copiedRef = mutexRef;
    mutexRef.lock();
    CMS_TEST_CHECK(referencedMutex.locked);
    CMS_TEST_CHECK(referencedMutex.locks == 1);
    copiedRef.unlock();
    CMS_TEST_CHECK(!referencedMutex.locked);
    CMS_TEST_CHECK(referencedMutex.unlocks == 1);

    using ExternalQueue = cms::util::sync::SynchronizedQueue<
        cms::util::StaticQueue<int, 3>,
        cms::util::sync::MutexRef<CountingMutex>>;

    CountingMutex externalMutex;
    ExternalQueue synchronized{
        cms::util::sync::MutexRef<CountingMutex>(externalMutex)};

    CMS_TEST_CHECK(synchronized.capacity() == 3);
    CMS_TEST_CHECK(externalMutex.locks == 0);
    CMS_TEST_CHECK(externalMutex.unlocks == 0);

    int locksBefore = externalMutex.locks;
    int unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.empty());
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.push(1) == cms::util::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.emplace(2) == cms::util::Status::ok);
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
    }) == cms::util::Status::ok);
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
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(consumed == 2);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.push(3) == cms::util::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.pop() == cms::util::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    bool emptyConsumerCalled = false;
    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int&) {
        emptyConsumerCalled = true;
    }) == cms::util::Status::out_of_range);
    CMS_TEST_CHECK(!emptyConsumerCalled);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.pop() == cms::util::Status::out_of_range);
    checkOperation(externalMutex, locksBefore, unlocksBefore);

    CMS_TEST_CHECK(synchronized.push(10) == cms::util::Status::ok);
    CMS_TEST_CHECK(synchronized.push(20) == cms::util::Status::ok);
    CMS_TEST_CHECK(synchronized.emplace(30) == cms::util::Status::ok);
    CMS_TEST_CHECK(synchronized.size() == 3);
    CMS_TEST_CHECK(synchronized.full());

    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(synchronized.emplace(40) == cms::util::Status::no_space);
    checkOperation(externalMutex, locksBefore, unlocksBefore);
    CMS_TEST_CHECK(synchronized.size() == 3);
    CMS_TEST_CHECK(synchronized.full());

    int preservedFront = 0;
    CMS_TEST_CHECK(synchronized.consumeFront([&](int& value) {
        preservedFront = value;
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(preservedFront == 10);

    ExternalQueue overwriteSynchronized{
        cms::util::sync::MutexRef<CountingMutex>(externalMutex)};
    CMS_TEST_REQUIRE(overwriteSynchronized.push(1) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwriteSynchronized.push(2) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwriteSynchronized.push(3) == cms::util::Status::ok);
    locksBefore = externalMutex.locks;
    unlocksBefore = externalMutex.unlocks;
    CMS_TEST_CHECK(overwriteSynchronized.pushOverwrite(4)
        == cms::util::Status::ok);
    checkOperation(externalMutex, locksBefore, unlocksBefore);
    CMS_TEST_CHECK(overwriteSynchronized.size() == 3);
    CMS_TEST_CHECK(overwriteSynchronized.full());
    for (int expected = 2; expected <= 4; ++expected) {
        int actual = 0;
        CMS_TEST_REQUIRE(overwriteSynchronized.consumeFront(
            [&actual](int& value) {
                actual = value;
            }) == cms::util::Status::ok);
        CMS_TEST_CHECK(actual == expected);
    }
    CMS_TEST_CHECK(overwriteSynchronized.empty());

    using NullQueue = cms::util::sync::SynchronizedQueue<
        cms::util::StaticQueue<int, 3>,
        cms::util::sync::NullMutex>;

    NullQueue nullQueue;
    CMS_TEST_CHECK(nullQueue.capacity() == 3);
    CMS_TEST_CHECK(nullQueue.empty());
    CMS_TEST_CHECK(nullQueue.push(10) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.emplace(20) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.size() == 2);
    CMS_TEST_CHECK(!nullQueue.empty());
    CMS_TEST_CHECK(!nullQueue.full());

    int nullConsumed = 0;
    CMS_TEST_CHECK(nullQueue.consumeFront([&](int& value) {
        nullConsumed = value;
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullConsumed == 10);
    CMS_TEST_CHECK(nullQueue.size() == 1);
    CMS_TEST_CHECK(nullQueue.pop() == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.empty());
    CMS_TEST_CHECK(nullQueue.push(30) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.push(40) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.push(50) == cms::util::Status::ok);
    CMS_TEST_CHECK(nullQueue.full());
    CMS_TEST_CHECK(nullQueue.push(60) == cms::util::Status::no_space);

    using MoveQueue = cms::util::sync::SynchronizedQueue<
        cms::util::StaticQueue<MoveOnly, 2>,
        cms::util::sync::NullMutex>;

    MoveQueue moveQueue;
    MoveOnly moveSource(71);
    CMS_TEST_CHECK(moveQueue.push(std::move(moveSource)) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveSource.value == -1);
    CMS_TEST_CHECK(moveQueue.emplace(72) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 71);
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 72);
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.empty());
    MoveOnly overwriteFirst(73);
    MoveOnly overwriteSecond(74);
    MoveOnly overwriteThird(75);
    CMS_TEST_REQUIRE(moveQueue.push(std::move(overwriteFirst))
        == cms::util::Status::ok);
    CMS_TEST_REQUIRE(moveQueue.push(std::move(overwriteSecond))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.pushOverwrite(std::move(overwriteThird))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 74);
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.consumeFront([](MoveOnly& value) {
        CMS_TEST_CHECK(value.value == 75);
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveQueue.empty());

    using NonDefaultQueue = cms::util::sync::SynchronizedQueue<
        cms::util::StaticQueue<NonDefault, 2>,
        cms::util::sync::NullMutex>;

    NonDefaultQueue nonDefaultQueue;
    CMS_TEST_CHECK(nonDefaultQueue.emplace(81) == cms::util::Status::ok);
    CMS_TEST_CHECK(nonDefaultQueue.consumeFront([](NonDefault& value) {
        CMS_TEST_CHECK(value.value == 81);
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(nonDefaultQueue.empty());

    int destructorCount = 0;
    using LifetimeQueue = cms::util::sync::SynchronizedQueue<
        cms::util::StaticQueue<LifetimeTracker, 2>,
        cms::util::sync::NullMutex>;

    LifetimeQueue lifetimeQueue;
    CMS_TEST_CHECK(lifetimeQueue.emplace(91, destructorCount) == cms::util::Status::ok);
    CMS_TEST_CHECK(destructorCount == 0);
    bool consumerRan = false;
    CMS_TEST_CHECK(lifetimeQueue.consumeFront([&](LifetimeTracker& value) {
        CMS_TEST_CHECK(value.value == 91);
        CMS_TEST_CHECK(destructorCount == 0);
        consumerRan = true;
    }) == cms::util::Status::ok);
    CMS_TEST_CHECK(consumerRan);
    CMS_TEST_CHECK(destructorCount == 1);
    CMS_TEST_CHECK(lifetimeQueue.empty());

    std::printf(
        "sizeof(cms::util::StaticQueue<std::uint8_t, 8>)=%zu\n",
        sizeof(cms::util::StaticQueue<std::uint8_t, 8>));
    std::printf(
        "sizeof(cms::util::sync::SynchronizedQueue<StaticQueue<uint8_t, 8>, "
        "NullMutex>)=%zu\n",
        sizeof(cms::util::sync::SynchronizedQueue<
            cms::util::StaticQueue<std::uint8_t, 8>,
            cms::util::sync::NullMutex>));
    std::printf(
        "sizeof(cms::util::sync::MutexRef<CountingMutex>)=%zu\n",
        sizeof(cms::util::sync::MutexRef<CountingMutex>));

    return cms::test::finish();
}
