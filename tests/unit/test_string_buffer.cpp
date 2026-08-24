#include <cstdio>
#include <utility>

#include <cms/string_buffer.h>

#include "test.h"

int main() {
    cms::StringBuffer defaultBuffer;
    CMS_TEST_CHECK(!defaultBuffer.valid());
    CMS_TEST_CHECK(defaultBuffer.data() == nullptr);
    CMS_TEST_CHECK(defaultBuffer.size() == 0);
    CMS_TEST_CHECK(defaultBuffer.capacity() == 0);
    CMS_TEST_CHECK(defaultBuffer.maxSize() == 0);
    CMS_TEST_CHECK(defaultBuffer.remaining() == 0);
    CMS_TEST_CHECK(defaultBuffer.empty());
    CMS_TEST_CHECK(defaultBuffer.view().empty());
    CMS_TEST_CHECK(defaultBuffer.clear() == cms::Status::invalid_argument);
    CMS_TEST_CHECK(defaultBuffer.commit(0) == cms::Status::invalid_argument);

    char storage[8] = "abc";
    std::size_t sharedSize = 3;
    cms::StringBuffer first(storage, sizeof(storage), sharedSize);

    CMS_TEST_REQUIRE(first.valid());
    CMS_TEST_CHECK(first.data() == storage);
    CMS_TEST_CHECK(first.size() == 3);
    CMS_TEST_CHECK(first.capacity() == 8);
    CMS_TEST_CHECK(first.maxSize() == 7);
    CMS_TEST_CHECK(first.remaining() == 4);
    CMS_TEST_CHECK(!first.empty());
    CMS_TEST_CHECK(first.data()[first.size()] == '\0');

    cms::StringBuffer second = first;
    cms::StringBuffer third;
    third = first;
    cms::StringBuffer moved = std::move(second);

    storage[0] = 'x';
    storage[1] = 'y';
    storage[2] = '\0';
    sharedSize = 2;

    CMS_TEST_CHECK(first.size() == 2);
    CMS_TEST_CHECK(second.size() == 2);
    CMS_TEST_CHECK(third.size() == 2);
    CMS_TEST_CHECK(moved.size() == 2);
    CMS_TEST_CHECK(first.data() == second.data());
    CMS_TEST_CHECK(first.data() == third.data());
    CMS_TEST_CHECK(first.data() == moved.data());

    const cms::StringView snapshot = first.view();
    CMS_TEST_REQUIRE(snapshot.data() == storage);
    CMS_TEST_CHECK(snapshot.size() == 2);

    CMS_TEST_CHECK(third.clear() == cms::Status::ok);
    CMS_TEST_REQUIRE(first.valid());
    CMS_TEST_CHECK(first.size() == 0);
    CMS_TEST_CHECK(second.size() == 0);
    CMS_TEST_CHECK(third.size() == 0);
    CMS_TEST_CHECK(moved.size() == 0);
    CMS_TEST_CHECK(storage[0] == '\0');
    CMS_TEST_CHECK(snapshot.data() == storage);
    CMS_TEST_CHECK(snapshot.size() == 2);
    CMS_TEST_CHECK(snapshot[0] == '\0');
    CMS_TEST_CHECK(first.view().size() == 0);

    char commitStorage[6] = "old";
    std::size_t commitSize = 3;
    cms::StringBuffer commitBuffer(
        commitStorage,
        sizeof(commitStorage),
        commitSize);
    CMS_TEST_REQUIRE(commitBuffer.valid());
    commitStorage[0] = 'n';
    commitStorage[1] = 'e';
    commitStorage[2] = 'w';
    commitStorage[3] = 'x';
    CMS_TEST_CHECK(commitBuffer.commit(3) == cms::Status::ok);
    CMS_TEST_REQUIRE(commitBuffer.valid());
    CMS_TEST_CHECK(commitSize == 3);
    CMS_TEST_CHECK(commitStorage[3] == '\0');

    commitStorage[5] = 'q';
    CMS_TEST_CHECK(
        commitBuffer.commit(sizeof(commitStorage)) == cms::Status::no_space);
    CMS_TEST_CHECK(commitSize == 3);
    CMS_TEST_CHECK(commitStorage[0] == 'n');
    CMS_TEST_CHECK(commitStorage[1] == 'e');
    CMS_TEST_CHECK(commitStorage[2] == 'w');
    CMS_TEST_CHECK(commitStorage[3] == '\0');
    CMS_TEST_CHECK(commitStorage[5] == 'q');

    commitSize = sizeof(commitStorage);
    CMS_TEST_CHECK(!commitBuffer.valid());
    commitStorage[0] = 'o';
    commitStorage[1] = 'k';
    commitStorage[2] = 'x';
    CMS_TEST_CHECK(commitBuffer.commit(2) == cms::Status::ok);
    CMS_TEST_REQUIRE(commitBuffer.valid());
    CMS_TEST_CHECK(commitSize == 2);
    CMS_TEST_CHECK(commitStorage[2] == '\0');

    char fullStorage[4] = "abc";
    std::size_t fullSize = 3;
    cms::StringBuffer full(fullStorage, sizeof(fullStorage), fullSize);
    CMS_TEST_REQUIRE(full.valid());
    CMS_TEST_CHECK(full.size() == full.maxSize());
    CMS_TEST_CHECK(full.remaining() == 0);

    std::size_t nullSize = 0;
    cms::StringBuffer nullZero(nullptr, 0, nullSize);
    cms::StringBuffer nullCapacity(nullptr, 10, nullSize);
    CMS_TEST_CHECK(!nullZero.valid());
    CMS_TEST_CHECK(!nullCapacity.valid());
    CMS_TEST_CHECK(nullZero.data() == nullptr);
    CMS_TEST_CHECK(nullCapacity.capacity() == 0);

    char zeroCapacityStorage[1] = "";
    cms::StringBuffer zeroCapacity(zeroCapacityStorage, 0, nullSize);
    CMS_TEST_CHECK(!zeroCapacity.valid());
    CMS_TEST_CHECK(zeroCapacity.data() == nullptr);

    char tooLargeStorage[4] = "abc";
    std::size_t tooLargeSize = sizeof(tooLargeStorage);
    cms::StringBuffer tooLarge(
        tooLargeStorage,
        sizeof(tooLargeStorage),
        tooLargeSize);
    CMS_TEST_CHECK(!tooLarge.valid());
    CMS_TEST_CHECK(tooLarge.capacity() == 0);

    char missingNulStorage[4] = {'a', 'b', 'c', 'x'};
    std::size_t missingNulSize = 3;
    cms::StringBuffer missingNul(
        missingNulStorage,
        sizeof(missingNulStorage),
        missingNulSize);
    CMS_TEST_CHECK(!missingNul.valid());
    CMS_TEST_CHECK(missingNul.data() == nullptr);

    char repairStorage[4] = "a";
    std::size_t repairSize = 1;
    cms::StringBuffer repair(repairStorage, sizeof(repairStorage), repairSize);
    CMS_TEST_REQUIRE(repair.valid());
    repairSize = sizeof(repairStorage);
    CMS_TEST_CHECK(!repair.valid());
    CMS_TEST_CHECK(repair.remaining() == 0);
    CMS_TEST_CHECK(repair.view().empty());
    CMS_TEST_CHECK(repair.clear() == cms::Status::ok);
    CMS_TEST_REQUIRE(repair.valid());
    CMS_TEST_CHECK(repairSize == 0);
    CMS_TEST_CHECK(repairStorage[0] == '\0');

    std::printf("sizeof(cms::StringBuffer)=%zu\n", sizeof(cms::StringBuffer));
    return cms::test::finish();
}
