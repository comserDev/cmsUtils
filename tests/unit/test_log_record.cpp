#include <cstddef>
#include <cstdio>
#include <utility>

#include <cms/log/record.h>
#include <cms/static_queue.h>

#include "test.h"

namespace {

void checkResult(
    const cms::WriteResult& result,
    cms::Status status,
    std::size_t written,
    std::size_t required) {
    CMS_TEST_CHECK(result.status == status);
    CMS_TEST_CHECK(result.written == written);
    CMS_TEST_CHECK(result.required == required);
}

void checkBytes(
    cms::StringView actual,
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
    cms::log::StaticRecord<8> record;
    CMS_TEST_CHECK(record.messageCapacity() == 8);
    CMS_TEST_CHECK(record.maxMessageSize() == 7);
    CMS_TEST_CHECK(record.level() == cms::log::Level::info);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 0);
    CMS_TEST_CHECK(record.message().empty());
    CMS_TEST_REQUIRE(record.message().data() != nullptr);

    checkResult(
        record.assign(cms::log::Level::warning, 42, cms::StringView("abc")),
        cms::Status::ok,
        3,
        3);
    CMS_TEST_CHECK(record.level() == cms::log::Level::warning);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 42);
    checkBytes(record.message(), "abc", 3);

    const cms::log::Record view = record.view();
    CMS_TEST_CHECK(view.level == record.level());
    CMS_TEST_CHECK(
        view.timestampMilliseconds == record.timestampMilliseconds());
    CMS_TEST_CHECK(view.message.data() == record.message().data());
    checkBytes(view.message, "abc", 3);

    checkResult(
        record.assign(cms::log::Level::critical, 99, cms::StringView("1234567")),
        cms::Status::ok,
        7,
        7);
    checkBytes(record.message(), "1234567", 7);

    checkResult(
        record.assign(cms::log::Level::trace, 100, cms::StringView("12345678")),
        cms::Status::no_space,
        0,
        8);
    CMS_TEST_CHECK(record.level() == cms::log::Level::critical);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 99);
    checkBytes(record.message(), "1234567", 7);

    checkResult(
        record.assign(cms::log::Level::debug, 7, cms::StringView()),
        cms::Status::ok,
        0,
        0);
    CMS_TEST_CHECK(record.level() == cms::log::Level::debug);
    CMS_TEST_CHECK(record.timestampMilliseconds() == 7);
    CMS_TEST_CHECK(record.message().empty());

    cms::log::StaticRecord<1> tiny;
    CMS_TEST_CHECK(tiny.messageCapacity() == 1);
    CMS_TEST_CHECK(tiny.maxMessageSize() == 0);
    checkResult(
        tiny.assign(cms::log::Level::error, 11, cms::StringView()),
        cms::Status::ok,
        0,
        0);
    CMS_TEST_CHECK(tiny.level() == cms::log::Level::error);
    CMS_TEST_CHECK(tiny.timestampMilliseconds() == 11);
    checkResult(
        tiny.assign(cms::log::Level::trace, 12, cms::StringView("x")),
        cms::Status::no_space,
        0,
        1);
    CMS_TEST_CHECK(tiny.level() == cms::log::Level::error);
    CMS_TEST_CHECK(tiny.timestampMilliseconds() == 11);
    CMS_TEST_CHECK(tiny.message().empty());

    const char embedded[] = {'A', '\0', 'B'};
    cms::log::StaticRecord<4> embeddedRecord;
    checkResult(
        embeddedRecord.assign(
            cms::log::Level::info,
            123,
            cms::StringView(embedded, sizeof(embedded))),
        cms::Status::ok,
        sizeof(embedded),
        sizeof(embedded));
    checkBytes(embeddedRecord.message(), embedded, sizeof(embedded));

    cms::log::StaticRecord<8> original;
    CMS_TEST_REQUIRE(
        original.assign(cms::log::Level::warning, 55, cms::StringView("copy"))
            .status == cms::Status::ok);
    cms::log::StaticRecord<8> copied(original);
    CMS_TEST_CHECK(copied.level() == cms::log::Level::warning);
    CMS_TEST_CHECK(copied.timestampMilliseconds() == 55);
    CMS_TEST_CHECK(copied.message().data() != original.message().data());
    checkBytes(copied.message(), "copy", 4);

    cms::log::StaticRecord<8> copyAssigned;
    copyAssigned = original;
    CMS_TEST_CHECK(copyAssigned.message().data() != original.message().data());
    checkBytes(copyAssigned.message(), "copy", 4);

    cms::log::StaticRecord<8> moveSource;
    CMS_TEST_REQUIRE(
        moveSource.assign(cms::log::Level::error, 66, cms::StringView("move"))
            .status == cms::Status::ok);
    const char* const moveSourceStorage = moveSource.message().data();
    cms::log::StaticRecord<8> moved(std::move(moveSource));
    CMS_TEST_CHECK(moved.level() == cms::log::Level::error);
    CMS_TEST_CHECK(moved.timestampMilliseconds() == 66);
    CMS_TEST_CHECK(moved.message().data() != moveSourceStorage);
    checkBytes(moved.message(), "move", 4);
    CMS_TEST_CHECK(moveSource.message().empty());
    CMS_TEST_CHECK(moveSource.message().data() == moveSourceStorage);

    cms::log::StaticRecord<8> moveAssigned;
    moveAssigned = std::move(moved);
    checkBytes(moveAssigned.message(), "move", 4);
    CMS_TEST_CHECK(moved.message().empty());

    std::printf("sizeof(cms::log::Record)=%zu\n", sizeof(cms::log::Record));
    std::printf(
        "sizeof(cms::log::StaticRecord<64>)=%zu\n",
        sizeof(cms::log::StaticRecord<64>));
    std::printf(
        "sizeof(cms::StaticQueue<cms::log::StaticRecord<64>, 8>)=%zu\n",
        sizeof(cms::StaticQueue<cms::log::StaticRecord<64>, 8>));

    return cms::test::finish();
}
