#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

#include <cms/util/static_queue.h>

#include "test.h"

namespace {

struct NonDefault {
    NonDefault() = delete;

    explicit NonDefault(int initialValue) noexcept
        : value(initialValue) {}

    int value;
};

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

struct TrackerCounts {
    int direct;
    int copies;
    int moves;
    int destructors;
    int live;
};

struct Tracker {
    Tracker(TrackerCounts& state, int initialValue) noexcept
        : counts(&state), value(initialValue) {
        ++counts->direct;
        ++counts->live;
    }

    Tracker(const Tracker& other) noexcept
        : counts(other.counts), value(other.value) {
        ++counts->copies;
        ++counts->live;
    }

    Tracker(Tracker&& other) noexcept
        : counts(other.counts), value(other.value) {
        other.value = -1;
        ++counts->moves;
        ++counts->live;
    }

    ~Tracker() noexcept {
        ++counts->destructors;
        --counts->live;
    }

    TrackerCounts* counts;
    int value;
};

struct AliasTracker {
    AliasTracker(TrackerCounts& state, int initialValue) noexcept
        : counts(&state), value(initialValue) {
        ++counts->direct;
        ++counts->live;
    }

    AliasTracker(const AliasTracker& other) noexcept
        : counts(other.counts), value(other.value) {
        ++counts->copies;
        ++counts->live;
    }

    AliasTracker(AliasTracker&& other) noexcept
        : counts(other.counts), value(other.value) {
        other.value = -1;
        ++counts->moves;
        ++counts->live;
    }

    ~AliasTracker() noexcept {
        ++counts->destructors;
        --counts->live;
    }

    // alias 검사가 overloaded operator& 대신 실제 object 주소를 쓰는지 확인한다.
    AliasTracker* operator&() noexcept {
        return nullptr;
    }

    const AliasTracker* operator&() const noexcept {
        return nullptr;
    }

    TrackerCounts* counts;
    int value;
};

struct OrderTracker {
    OrderTracker(
        int initialValue,
        int* destructionOrder,
        std::size_t& destructionCount) noexcept
        : value(initialValue),
          order(destructionOrder),
          count(&destructionCount) {}

    ~OrderTracker() noexcept {
        order[*count] = value;
        ++(*count);
    }

    int value;
    int* order;
    std::size_t* count;
};

struct alignas(32) OverAligned {
    explicit OverAligned(int initialValue) noexcept
        : value(static_cast<std::uint32_t>(initialValue)), padding{} {}

    std::uint32_t value;
    unsigned char padding[28];
};

template<std::size_t Capacity>
void checkOverwriteBoundary() {
    cms::util::StaticQueue<std::uint16_t, Capacity> queue;
    for (std::size_t index = 0; index < Capacity; ++index) {
        CMS_TEST_REQUIRE(queue.push(static_cast<std::uint16_t>(index))
            == cms::util::Status::ok);
    }

    CMS_TEST_CHECK(queue.full());
    CMS_TEST_CHECK(queue.size() == Capacity);
    CMS_TEST_CHECK(queue.pushOverwrite(UINT16_C(999)) == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.full());
    CMS_TEST_CHECK(queue.size() == Capacity);
    CMS_TEST_REQUIRE(queue.front() != nullptr);
    const std::uint16_t expected = Capacity == 1
        ? UINT16_C(999)
        : UINT16_C(1);
    CMS_TEST_CHECK(*queue.front() == expected);
}

} // namespace

int main() {
    cms::util::StaticQueue<int, 3> queue;
    CMS_TEST_CHECK(queue.size() == 0);
    CMS_TEST_CHECK(queue.capacity() == 3);
    CMS_TEST_CHECK(queue.empty());
    CMS_TEST_CHECK(!queue.full());
    CMS_TEST_CHECK(queue.front() == nullptr);
    CMS_TEST_CHECK(queue.pop() == cms::util::Status::out_of_range);

    CMS_TEST_CHECK(queue.push(1) == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.push(2) == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.emplace(3) == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.size() == 3);
    CMS_TEST_CHECK(queue.full());
    CMS_TEST_REQUIRE(queue.front() != nullptr);
    CMS_TEST_CHECK(*queue.front() == 1);
    CMS_TEST_CHECK(queue.push(4) == cms::util::Status::no_space);
    CMS_TEST_CHECK(queue.size() == 3);
    CMS_TEST_CHECK(*queue.front() == 1);

    CMS_TEST_CHECK(queue.pop() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(queue.front() != nullptr);
    CMS_TEST_CHECK(*queue.front() == 2);
    CMS_TEST_CHECK(queue.push(4) == cms::util::Status::ok);

    const cms::util::StaticQueue<int, 3>& constQueue = queue;
    CMS_TEST_REQUIRE(constQueue.front() != nullptr);
    CMS_TEST_CHECK(*constQueue.front() == 2);

    CMS_TEST_CHECK(queue.pop() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(queue.front() != nullptr);
    CMS_TEST_CHECK(*queue.front() == 3);
    CMS_TEST_CHECK(queue.pop() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(queue.front() != nullptr);
    CMS_TEST_CHECK(*queue.front() == 4);
    CMS_TEST_CHECK(queue.pop() == cms::util::Status::ok);
    CMS_TEST_CHECK(queue.empty());
    CMS_TEST_CHECK(queue.front() == nullptr);

    cms::util::StaticQueue<int, 1> single;
    CMS_TEST_CHECK(single.empty());
    CMS_TEST_CHECK(single.push(10) == cms::util::Status::ok);
    CMS_TEST_CHECK(single.full());
    CMS_TEST_CHECK(single.push(20) == cms::util::Status::no_space);
    CMS_TEST_REQUIRE(single.front() != nullptr);
    CMS_TEST_CHECK(*single.front() == 10);
    CMS_TEST_CHECK(single.pop() == cms::util::Status::ok);
    CMS_TEST_CHECK(single.empty());
    CMS_TEST_CHECK(single.push(30) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(single.front() != nullptr);
    CMS_TEST_CHECK(*single.front() == 30);
    CMS_TEST_CHECK(single.pop() == cms::util::Status::ok);
    CMS_TEST_CHECK(single.push(40) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(single.front() != nullptr);
    CMS_TEST_CHECK(*single.front() == 40);

    cms::util::StaticQueue<int, 3> overwrite;
    CMS_TEST_CHECK(overwrite.pushOverwrite(1) == cms::util::Status::ok);
    CMS_TEST_CHECK(overwrite.pushOverwrite(2) == cms::util::Status::ok);
    CMS_TEST_CHECK(overwrite.pushOverwrite(3) == cms::util::Status::ok);
    CMS_TEST_CHECK(overwrite.full());
    CMS_TEST_CHECK(overwrite.pushOverwrite(4) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwrite.front() != nullptr);
    CMS_TEST_CHECK(*overwrite.front() == 2);
    CMS_TEST_CHECK(overwrite.pushOverwrite(5) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwrite.front() != nullptr);
    CMS_TEST_CHECK(*overwrite.front() == 3);
    for (int expectedValue = 3; expectedValue <= 5; ++expectedValue) {
        CMS_TEST_REQUIRE(overwrite.front() != nullptr);
        CMS_TEST_CHECK(*overwrite.front() == expectedValue);
        CMS_TEST_REQUIRE(overwrite.pop() == cms::util::Status::ok);
    }
    CMS_TEST_CHECK(overwrite.empty());

    cms::util::StaticQueue<int, 1> copyOverwrite;
    const int copiedFirst = 61;
    const int copiedSecond = 62;
    CMS_TEST_REQUIRE(copyOverwrite.pushOverwrite(copiedFirst)
        == cms::util::Status::ok);
    CMS_TEST_CHECK(copyOverwrite.pushOverwrite(copiedSecond)
        == cms::util::Status::ok);
    CMS_TEST_REQUIRE(copyOverwrite.front() != nullptr);
    CMS_TEST_CHECK(*copyOverwrite.front() == copiedSecond);

    checkOverwriteBoundary<1>();
    checkOverwriteBoundary<255>();
    checkOverwriteBoundary<256>();

    cms::util::StaticQueue<NonDefault, 3> nonDefault;
    CMS_TEST_CHECK(nonDefault.empty());
    CMS_TEST_CHECK(nonDefault.emplace(11) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(nonDefault.front() != nullptr);
    CMS_TEST_CHECK(nonDefault.front()->value == 11);

    cms::util::StaticQueue<MoveOnly, 3> moveOnly;
    MoveOnly source(21);
    CMS_TEST_CHECK(moveOnly.push(std::move(source)) == cms::util::Status::ok);
    CMS_TEST_CHECK(source.value == -1);
    CMS_TEST_REQUIRE(moveOnly.front() != nullptr);
    CMS_TEST_CHECK(moveOnly.front()->value == 21);
    CMS_TEST_CHECK(moveOnly.emplace(22) == cms::util::Status::ok);
    CMS_TEST_CHECK(moveOnly.pop() == cms::util::Status::ok);
    CMS_TEST_REQUIRE(moveOnly.front() != nullptr);
    CMS_TEST_CHECK(moveOnly.front()->value == 22);

    TrackerCounts lifetime{0, 0, 0, 0, 0};
    {
        cms::util::StaticQueue<Tracker, 3> tracked;
        CMS_TEST_CHECK(lifetime.direct == 0);
        CMS_TEST_CHECK(lifetime.live == 0);
        CMS_TEST_CHECK(tracked.emplace(lifetime, 1) == cms::util::Status::ok);
        CMS_TEST_CHECK(tracked.emplace(lifetime, 2) == cms::util::Status::ok);
        CMS_TEST_CHECK(tracked.emplace(lifetime, 3) == cms::util::Status::ok);
        CMS_TEST_CHECK(lifetime.direct == 3);
        CMS_TEST_CHECK(lifetime.live == 3);
        CMS_TEST_CHECK(tracked.pop() == cms::util::Status::ok);
        CMS_TEST_CHECK(lifetime.destructors == 1);
        CMS_TEST_CHECK(lifetime.live == 2);
        tracked.clear();
        CMS_TEST_CHECK(lifetime.destructors == 3);
        CMS_TEST_CHECK(lifetime.live == 0);
        CMS_TEST_CHECK(tracked.empty());
        CMS_TEST_CHECK(!tracked.full());
        CMS_TEST_CHECK(tracked.front() == nullptr);
        CMS_TEST_CHECK(tracked.pop() == cms::util::Status::out_of_range);
        CMS_TEST_CHECK(lifetime.destructors == 3);
    }
    CMS_TEST_CHECK(lifetime.destructors == 3);
    CMS_TEST_CHECK(lifetime.live == 0);

    TrackerCounts copyMove{0, 0, 0, 0, 0};
    {
        Tracker sourceTracker(copyMove, 7);
        cms::util::StaticQueue<Tracker, 2> tracked;
        CMS_TEST_CHECK(tracked.push(sourceTracker) == cms::util::Status::ok);
        CMS_TEST_CHECK(copyMove.copies == 1);
        CMS_TEST_REQUIRE(tracked.front() != nullptr);
        CMS_TEST_CHECK(tracked.front()->value == 7);
        CMS_TEST_CHECK(tracked.pop() == cms::util::Status::ok);
        CMS_TEST_CHECK(tracked.push(std::move(sourceTracker)) == cms::util::Status::ok);
        CMS_TEST_CHECK(copyMove.moves == 1);
        CMS_TEST_REQUIRE(tracked.front() != nullptr);
        CMS_TEST_CHECK(tracked.front()->value == 7);
    }
    CMS_TEST_CHECK(copyMove.live == 0);
    CMS_TEST_CHECK(copyMove.destructors == 3);

    TrackerCounts fullCounts{0, 0, 0, 0, 0};
    {
        cms::util::StaticQueue<Tracker, 1> fullQueue;
        CMS_TEST_CHECK(fullQueue.emplace(fullCounts, 1) == cms::util::Status::ok);
        Tracker fullSource(fullCounts, 2);
        const int directBefore = fullCounts.direct;
        const int copiesBefore = fullCounts.copies;
        const int movesBefore = fullCounts.moves;
        const int destructorsBefore = fullCounts.destructors;
        const int liveBefore = fullCounts.live;

        CMS_TEST_CHECK(fullQueue.push(fullSource) == cms::util::Status::no_space);
        CMS_TEST_CHECK(
            fullQueue.push(std::move(fullSource)) == cms::util::Status::no_space);
        CMS_TEST_CHECK(
            fullQueue.emplace(fullCounts, 3) == cms::util::Status::no_space);
        CMS_TEST_CHECK(fullCounts.direct == directBefore);
        CMS_TEST_CHECK(fullCounts.copies == copiesBefore);
        CMS_TEST_CHECK(fullCounts.moves == movesBefore);
        CMS_TEST_CHECK(fullCounts.destructors == destructorsBefore);
        CMS_TEST_CHECK(fullCounts.live == liveBefore);
        CMS_TEST_REQUIRE(fullQueue.front() != nullptr);
        CMS_TEST_CHECK(fullQueue.front()->value == 1);
    }
    CMS_TEST_CHECK(fullCounts.live == 0);
    CMS_TEST_CHECK(fullCounts.destructors == 2);

    TrackerCounts overwriteCounts{0, 0, 0, 0, 0};
    {
        cms::util::StaticQueue<Tracker, 3> tracked;
        CMS_TEST_REQUIRE(tracked.emplace(overwriteCounts, 1)
            == cms::util::Status::ok);
        CMS_TEST_REQUIRE(tracked.emplace(overwriteCounts, 2)
            == cms::util::Status::ok);
        CMS_TEST_REQUIRE(tracked.emplace(overwriteCounts, 3)
            == cms::util::Status::ok);
        Tracker replacement(overwriteCounts, 4);
        const int destructorsBefore = overwriteCounts.destructors;
        CMS_TEST_CHECK(tracked.pushOverwrite(std::move(replacement))
            == cms::util::Status::ok);
        CMS_TEST_CHECK(overwriteCounts.destructors == destructorsBefore + 1);
        CMS_TEST_CHECK(overwriteCounts.moves == 1);
        CMS_TEST_CHECK(overwriteCounts.live == 4);
        CMS_TEST_REQUIRE(tracked.front() != nullptr);
        CMS_TEST_CHECK(tracked.front()->value == 2);
    }
    CMS_TEST_CHECK(overwriteCounts.live == 0);
    CMS_TEST_CHECK(
        overwriteCounts.destructors
        == overwriteCounts.direct
            + overwriteCounts.copies
            + overwriteCounts.moves);

    TrackerCounts aliasCounts{0, 0, 0, 0, 0};
    {
        cms::util::StaticQueue<AliasTracker, 3> aliasQueue;
        CMS_TEST_REQUIRE(aliasQueue.emplace(aliasCounts, 1)
            == cms::util::Status::ok);
        CMS_TEST_REQUIRE(aliasQueue.emplace(aliasCounts, 2)
            == cms::util::Status::ok);
        CMS_TEST_REQUIRE(aliasQueue.emplace(aliasCounts, 3)
            == cms::util::Status::ok);
        CMS_TEST_REQUIRE(aliasQueue.front() != nullptr);
        AliasTracker* const oldest = aliasQueue.front();

        CMS_TEST_CHECK(aliasQueue.pushOverwrite(*oldest)
            == cms::util::Status::invalid_argument);
        CMS_TEST_CHECK(aliasQueue.size() == 3);
        CMS_TEST_CHECK(aliasQueue.full());
        CMS_TEST_CHECK(aliasQueue.front() == oldest);
        CMS_TEST_CHECK(oldest->value == 1);
        CMS_TEST_CHECK(aliasCounts.direct == 3);
        CMS_TEST_CHECK(aliasCounts.copies == 0);
        CMS_TEST_CHECK(aliasCounts.moves == 0);
        CMS_TEST_CHECK(aliasCounts.destructors == 0);
        CMS_TEST_CHECK(aliasCounts.live == 3);

        CMS_TEST_CHECK(aliasQueue.pushOverwrite(std::move(*oldest))
            == cms::util::Status::invalid_argument);
        CMS_TEST_CHECK(aliasQueue.size() == 3);
        CMS_TEST_CHECK(aliasQueue.full());
        CMS_TEST_CHECK(aliasQueue.front() == oldest);
        CMS_TEST_CHECK(oldest->value == 1);
        CMS_TEST_CHECK(aliasCounts.direct == 3);
        CMS_TEST_CHECK(aliasCounts.copies == 0);
        CMS_TEST_CHECK(aliasCounts.moves == 0);
        CMS_TEST_CHECK(aliasCounts.destructors == 0);
        CMS_TEST_CHECK(aliasCounts.live == 3);

        for (int expectedValue = 1; expectedValue <= 3; ++expectedValue) {
            CMS_TEST_REQUIRE(aliasQueue.front() != nullptr);
            CMS_TEST_CHECK(aliasQueue.front()->value == expectedValue);
            CMS_TEST_REQUIRE(aliasQueue.pop() == cms::util::Status::ok);
        }
        CMS_TEST_CHECK(aliasQueue.empty());
    }
    CMS_TEST_CHECK(aliasCounts.direct == 3);
    CMS_TEST_CHECK(aliasCounts.copies == 0);
    CMS_TEST_CHECK(aliasCounts.moves == 0);
    CMS_TEST_CHECK(aliasCounts.destructors == 3);
    CMS_TEST_CHECK(aliasCounts.live == 0);

    cms::util::StaticQueue<MoveOnly, 1> overwriteMoveOnly;
    MoveOnly firstMoveOnly(51);
    MoveOnly secondMoveOnly(52);
    CMS_TEST_CHECK(overwriteMoveOnly.push(std::move(firstMoveOnly))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(overwriteMoveOnly.pushOverwrite(std::move(secondMoveOnly))
        == cms::util::Status::ok);
    CMS_TEST_CHECK(firstMoveOnly.value == -1);
    CMS_TEST_CHECK(secondMoveOnly.value == -1);
    CMS_TEST_REQUIRE(overwriteMoveOnly.front() != nullptr);
    CMS_TEST_CHECK(overwriteMoveOnly.front()->value == 52);

    TrackerCounts destructorCounts{0, 0, 0, 0, 0};
    {
        cms::util::StaticQueue<Tracker, 3> tracked;
        CMS_TEST_CHECK(tracked.emplace(destructorCounts, 1) == cms::util::Status::ok);
        CMS_TEST_CHECK(tracked.emplace(destructorCounts, 2) == cms::util::Status::ok);
        CMS_TEST_CHECK(destructorCounts.live == 2);
    }
    CMS_TEST_CHECK(destructorCounts.live == 0);
    CMS_TEST_CHECK(destructorCounts.destructors == 2);

    int destructionOrder[4] = {};
    std::size_t destructionCount = 0;
    {
        cms::util::StaticQueue<OrderTracker, 3> orderedDestruction;
        CMS_TEST_CHECK(
            orderedDestruction.emplace(1, destructionOrder, destructionCount)
            == cms::util::Status::ok);
        CMS_TEST_CHECK(
            orderedDestruction.emplace(2, destructionOrder, destructionCount)
            == cms::util::Status::ok);
        CMS_TEST_CHECK(
            orderedDestruction.emplace(3, destructionOrder, destructionCount)
            == cms::util::Status::ok);
        CMS_TEST_CHECK(orderedDestruction.pop() == cms::util::Status::ok);
        CMS_TEST_CHECK(destructionCount == 1);
        CMS_TEST_CHECK(destructionOrder[0] == 1);
        CMS_TEST_CHECK(
            orderedDestruction.emplace(4, destructionOrder, destructionCount)
            == cms::util::Status::ok);
        orderedDestruction.clear();
        CMS_TEST_CHECK(destructionCount == 4);
        CMS_TEST_CHECK(destructionOrder[0] == 1);
        CMS_TEST_CHECK(destructionOrder[1] == 2);
        CMS_TEST_CHECK(destructionOrder[2] == 3);
        CMS_TEST_CHECK(destructionOrder[3] == 4);
    }
    CMS_TEST_CHECK(destructionCount == 4);

    cms::util::StaticQueue<int, 3> reusable;
    CMS_TEST_CHECK(reusable.emplace(1) == cms::util::Status::ok);
    CMS_TEST_CHECK(reusable.emplace(2) == cms::util::Status::ok);
    reusable.clear();
    CMS_TEST_CHECK(reusable.size() == 0);
    CMS_TEST_CHECK(reusable.empty());
    CMS_TEST_CHECK(reusable.front() == nullptr);
    CMS_TEST_CHECK(reusable.emplace(10) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(reusable.front() != nullptr);
    CMS_TEST_CHECK(*reusable.front() == 10);
    CMS_TEST_CHECK(reusable.pop() == cms::util::Status::ok);
    CMS_TEST_CHECK(reusable.empty());

    cms::util::StaticQueue<NonDefault, 2> stable;
    CMS_TEST_CHECK(stable.emplace(31) == cms::util::Status::ok);
    NonDefault* first = stable.front();
    CMS_TEST_REQUIRE(first != nullptr);
    CMS_TEST_CHECK(first->value == 31);
    CMS_TEST_CHECK(stable.emplace(32) == cms::util::Status::ok);
    CMS_TEST_CHECK(stable.front() == first);
    CMS_TEST_CHECK(first->value == 31);

    cms::util::StaticQueue<OverAligned, 2> aligned;
    CMS_TEST_CHECK(aligned.emplace(41) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(aligned.front() != nullptr);
    const std::uintptr_t address =
        reinterpret_cast<std::uintptr_t>(aligned.front());
    CMS_TEST_CHECK(address % alignof(OverAligned) == 0);
    CMS_TEST_CHECK(aligned.front()->value == 41);

    cms::util::StaticQueue<int, 7> cycles;
    for (int value = 0; value < 7; ++value) {
        CMS_TEST_REQUIRE(cycles.push(value) == cms::util::Status::ok);
    }
    int expected = 0;
    for (int value = 7; value < 4096; ++value) {
        CMS_TEST_REQUIRE(cycles.front() != nullptr);
        CMS_TEST_CHECK(*cycles.front() == expected);
        CMS_TEST_REQUIRE(cycles.pop() == cms::util::Status::ok);
        CMS_TEST_REQUIRE(cycles.push(value) == cms::util::Status::ok);
        ++expected;
    }
    while (expected < 4096) {
        CMS_TEST_REQUIRE(cycles.front() != nullptr);
        CMS_TEST_CHECK(*cycles.front() == expected);
        CMS_TEST_REQUIRE(cycles.pop() == cms::util::Status::ok);
        ++expected;
    }
    CMS_TEST_CHECK(cycles.empty());
    CMS_TEST_CHECK(cycles.size() == 0);

    cms::util::StaticQueue<int, 3> overwriteCycles;
    CMS_TEST_REQUIRE(overwriteCycles.push(0) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwriteCycles.push(1) == cms::util::Status::ok);
    CMS_TEST_REQUIRE(overwriteCycles.push(2) == cms::util::Status::ok);
    for (int value = 3; value < 1024; ++value) {
        CMS_TEST_REQUIRE(overwriteCycles.pushOverwrite(value)
            == cms::util::Status::ok);
    }
    for (int expectedValue = 1021; expectedValue < 1024; ++expectedValue) {
        CMS_TEST_REQUIRE(overwriteCycles.front() != nullptr);
        CMS_TEST_CHECK(*overwriteCycles.front() == expectedValue);
        CMS_TEST_REQUIRE(overwriteCycles.pop() == cms::util::Status::ok);
    }
    CMS_TEST_CHECK(overwriteCycles.empty());

    std::printf(
        "sizeof(cms::util::StaticQueue<std::uint8_t, 1>)=%zu\n",
        sizeof(cms::util::StaticQueue<std::uint8_t, 1>));
    std::printf(
        "sizeof(cms::util::StaticQueue<std::uint8_t, 8>)=%zu\n",
        sizeof(cms::util::StaticQueue<std::uint8_t, 8>));
    std::printf(
        "sizeof(cms::util::StaticQueue<std::uint8_t, 255>)=%zu\n",
        sizeof(cms::util::StaticQueue<std::uint8_t, 255>));
    std::printf(
        "sizeof(cms::util::StaticQueue<std::uint8_t, 256>)=%zu\n",
        sizeof(cms::util::StaticQueue<std::uint8_t, 256>));

    return cms::test::finish();
}
