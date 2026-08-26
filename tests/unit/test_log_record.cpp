#include <cstddef>
#include <cstdio>
#include <utility>

#include <cms/util/log/record.h>
#include <cms/util/static_queue.h>

#include "test.h"

namespace {

void checkResult(
    const cms::util::WriteResult& result,
    cms::util::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

void checkBytes(
    cms::util::StringView actual,
    const char* expected,
    std::size_t expectedSize) {
    CMS_TEST_REQUIRE(actual.size() == expectedSize);
    if (expectedSize > 0) {
        CMS_TEST_REQUIRE(actual.data() != nullptr);
        CMS_TEST_REQUIRE(expected != nullptr);
    }
    for (std::size_t index = 0; index < expectedSize; ++index) {
        CMS_TEST_CHECK(actual[index] == expected[index]);
    }
}

} // namespace

int main() {
    cms::util::log::StaticRecord<8> record;
    CMS_TEST_CHECK(record.messageCapacity() == 8);
    CMS_TEST_CHECK(record.maxMessageSize() == 7);
    CMS_TEST_CHECK(record.level() == cms::util::log::Level::info);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 0);
    CMS_TEST_CHECK(record.message().empty());
    CMS_TEST_REQUIRE(record.message().data() != nullptr);

    checkResult(
        record.assign(cms::util::log::Level::warning, 42, cms::util::StringView("abc")),
        cms::util::Status::ok,
        3,
        3);
    CMS_TEST_CHECK(record.level() == cms::util::log::Level::warning);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 42);
    checkBytes(record.message(), "abc", 3);

    const cms::util::log::Record view = record.view();
    CMS_TEST_CHECK(view.level == record.level());
    CMS_TEST_CHECK(
        view.timestampMilliseconds == record.timestampMilliseconds());
    CMS_TEST_CHECK(view.message.data() == record.message().data());
    checkBytes(view.message, "abc", 3);

    checkResult(
        record.assign(cms::util::log::Level::critical, 99, cms::util::StringView("1234567")),
        cms::util::Status::ok,
        7,
        7);
    checkBytes(record.message(), "1234567", 7);

    checkResult(
        record.assign(cms::util::log::Level::trace, 100, cms::util::StringView("12345678")),
        cms::util::Status::no_space,
        0,
        8);
    CMS_TEST_CHECK(record.level() == cms::util::log::Level::critical);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 99);
    checkBytes(record.message(), "1234567", 7);

    checkResult(
        record.assign(cms::util::log::Level::debug, 7, cms::util::StringView()),
        cms::util::Status::ok,
        0,
        0);
    CMS_TEST_CHECK(record.level() == cms::util::log::Level::debug);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 7);
    CMS_TEST_CHECK(record.message().empty());

    cms::util::log::StaticRecord<1> tiny;
    CMS_TEST_CHECK(tiny.messageCapacity() == 1);
    CMS_TEST_CHECK(tiny.maxMessageSize() == 0);
    checkResult(
        tiny.assign(cms::util::log::Level::error, 11, cms::util::StringView()),
        cms::util::Status::ok,
        0,
        0);
    CMS_TEST_CHECK(tiny.level() == cms::util::log::Level::error);
    CMS_TEST_CHECK(tiny.timestampMilliseconds() == 11);
    checkResult(
        tiny.assign(cms::util::log::Level::trace, 12, cms::util::StringView("x")),
        cms::util::Status::no_space,
        0,
        1);
    CMS_TEST_CHECK(tiny.level() == cms::util::log::Level::error);
    CMS_TEST_CHECK(tiny.timestampMilliseconds() == 11);
    CMS_TEST_CHECK(tiny.message().empty());

    const char embedded[] = {'A', '\0', 'B'};
    cms::util::log::StaticRecord<4> embeddedRecord;
    checkResult(
        embeddedRecord.assign(
            cms::util::log::Level::info,
            123,
            cms::util::StringView(embedded, sizeof(embedded))),
        cms::util::Status::ok,
        sizeof(embedded),
        sizeof(embedded));
    checkBytes(embeddedRecord.message(), embedded, sizeof(embedded));

    cms::util::log::StaticRecord<8> original;
    CMS_TEST_REQUIRE(
        original.assign(cms::util::log::Level::warning, 55, cms::util::StringView("copy"))
            .status == cms::util::Status::ok);
    cms::util::log::StaticRecord<8> copied(original);
    CMS_TEST_CHECK(copied.level() == cms::util::log::Level::warning);
    CMS_TEST_CHECK(copied.timestampMilliseconds() == 55);
    CMS_TEST_CHECK(copied.message().data() != original.message().data());
    checkBytes(copied.message(), "copy", 4);

    cms::util::log::StaticRecord<8> copyAssigned;
    copyAssigned = original;
    CMS_TEST_CHECK(copyAssigned.message().data() != original.message().data());
    checkBytes(copyAssigned.message(), "copy", 4);

    cms::util::log::StaticRecord<8> moveSource;
    CMS_TEST_REQUIRE(
        moveSource.assign(cms::util::log::Level::error, 66, cms::util::StringView("move"))
            .status == cms::util::Status::ok);
    const char* const moveSourceStorage = moveSource.message().data();
    cms::util::log::StaticRecord<8> moved(std::move(moveSource));
    CMS_TEST_CHECK(moved.level() == cms::util::log::Level::error);
    CMS_TEST_CHECK(moved.timestampMilliseconds() == 66);
    CMS_TEST_CHECK(moved.message().data() != moveSourceStorage);
    checkBytes(moved.message(), "move", 4);
    CMS_TEST_CHECK(moveSource.message().empty());
    CMS_TEST_CHECK(moveSource.message().data() == moveSourceStorage);

    cms::util::log::StaticRecord<8> moveAssigned;
    moveAssigned = std::move(moved);
    checkBytes(moveAssigned.message(), "move", 4);
    CMS_TEST_CHECK(moved.message().empty());

    std::printf("sizeof(cms::util::log::Record)=%zu\n", sizeof(cms::util::log::Record));
    std::printf(
        "sizeof(cms::util::log::StaticRecord<64>)=%zu\n",
        sizeof(cms::util::log::StaticRecord<64>));
    std::printf(
        "sizeof(cms::util::StaticQueue<cms::util::log::StaticRecord<64>, 8>)=%zu\n",
        sizeof(cms::util::StaticQueue<cms::util::log::StaticRecord<64>, 8>));

    return cms::test::finish();
}
