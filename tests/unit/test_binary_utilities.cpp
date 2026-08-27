#include <cstddef>
#include <cstdint>
#include <limits>

#include <cms/util/binary_reader.h>
#include <cms/util/binary_writer.h>
#include <cms/util/crc32.h>
#include <cms/util/static_byte_buffer.h>

#include "test.h"

using cms::util::BinaryReader;
using cms::util::BinaryWriter;
using cms::util::ByteBuffer;
using cms::util::ByteView;
using cms::util::StaticByteBuffer;
using cms::util::Status;

namespace {

void testByteStorage() {
    const ByteView empty;
    CMS_TEST_CHECK(empty.data() == nullptr);
    CMS_TEST_CHECK(empty.size() == 0);
    CMS_TEST_CHECK(empty.empty());

    const ByteView canonicalNull(nullptr, 7);
    CMS_TEST_CHECK(canonicalNull.data() == nullptr);
    CMS_TEST_CHECK(canonicalNull.size() == 0);

    const std::uint8_t bytes[] = {0x41, 0x00, 0x42};
    const ByteView view(bytes);
    CMS_TEST_CHECK(view.size() == 3);
    CMS_TEST_CHECK(view[1] == 0);
    CMS_TEST_CHECK(view.subview(1, 8).size() == 2);

    std::size_t zeroSize = 0;
    ByteBuffer zero(nullptr, 0, zeroSize);
    CMS_TEST_CHECK(zero.valid());
    CMS_TEST_CHECK(zero.remaining() == 0);
    CMS_TEST_CHECK(zero.commit(1) == Status::no_space);
    CMS_TEST_CHECK(zeroSize == 0);

    std::uint8_t oneByte[1] = {};
    std::size_t size = 0;
    ByteBuffer one(oneByte, 1, size);
    CMS_TEST_CHECK(one.commit(1) == Status::ok);
    CMS_TEST_CHECK(one.commit(2) == Status::no_space);
    CMS_TEST_CHECK(one.size() == 1);

    std::size_t invalidSize = 2;
    ByteBuffer invalid(oneByte, 1, invalidSize);
    CMS_TEST_CHECK(!invalid.valid());
    CMS_TEST_CHECK(invalid.clear() == Status::invalid_argument);
    CMS_TEST_CHECK(invalidSize == 2);
}

void testRoundTripAndUnalignedInput() {
    StaticByteBuffer<32> storage;
    BinaryWriter writer(storage.buffer());
    CMS_TEST_REQUIRE(writer.writeUint8(0xAB) == Status::ok);
    CMS_TEST_REQUIRE(writer.writeUint16BigEndian(0x1234) == Status::ok);
    CMS_TEST_REQUIRE(writer.writeUint32BigEndian(0x89ABCDEFU) == Status::ok);
    CMS_TEST_REQUIRE(writer.writeUint64BigEndian(0x0123456789ABCDEFULL) == Status::ok);

    std::uint8_t unaligned[16] = {};
    for (std::size_t i = 0; i < storage.size(); ++i) unaligned[i + 1] = storage.data()[i];
    BinaryReader reader(ByteView(unaligned + 1, storage.size()));
    std::uint8_t u8 = 0;
    std::uint16_t u16 = 0;
    std::uint32_t u32 = 0;
    std::uint64_t u64 = 0;
    CMS_TEST_CHECK(reader.readUint8(u8) == Status::ok && u8 == 0xAB);
    CMS_TEST_CHECK(reader.readUint16BigEndian(u16) == Status::ok && u16 == 0x1234);
    CMS_TEST_CHECK(reader.readUint32BigEndian(u32) == Status::ok && u32 == 0x89ABCDEFU);
    CMS_TEST_CHECK(reader.readUint64BigEndian(u64) == Status::ok && u64 == 0x0123456789ABCDEFULL);
    CMS_TEST_CHECK(reader.empty());
}

void testTransactionalFailuresAtEveryBoundary() {
    const std::uint8_t source[] = {1, 2, 3, 4, 5, 6, 7, 8};

    for (std::size_t available = 0; available < 1; ++available) {
        BinaryReader reader(ByteView(source, available));
        std::uint8_t value = 0xA5;
        CMS_TEST_CHECK(reader.readUint8(value) == Status::out_of_range);
        CMS_TEST_CHECK(reader.position() == 0 && value == 0xA5);
    }
    for (std::size_t available = 0; available < 2; ++available) {
        BinaryReader reader(ByteView(source, available));
        std::uint16_t value = 0xA5A5;
        CMS_TEST_CHECK(reader.readUint16BigEndian(value) == Status::out_of_range);
        CMS_TEST_CHECK(reader.position() == 0 && value == 0xA5A5);
    }
    for (std::size_t available = 0; available < 4; ++available) {
        BinaryReader reader(ByteView(source, available));
        std::uint32_t value = 0xA5A5A5A5U;
        CMS_TEST_CHECK(reader.readUint32BigEndian(value) == Status::out_of_range);
        CMS_TEST_CHECK(reader.position() == 0 && value == 0xA5A5A5A5U);
    }
    for (std::size_t available = 0; available < 8; ++available) {
        BinaryReader reader(ByteView(source, available));
        std::uint64_t value = 0xDEADBEEFDEADBEEFULL;
        CMS_TEST_CHECK(reader.readUint64BigEndian(value) == Status::out_of_range);
        CMS_TEST_CHECK(reader.position() == 0);
        CMS_TEST_CHECK(value == 0xDEADBEEFDEADBEEFULL);
    }

    for (std::size_t capacity = 0; capacity < 1; ++capacity) {
        std::uint8_t destination = 0xCC;
        std::size_t size = 0;
        BinaryWriter writer(ByteBuffer(nullptr, capacity, size));
        CMS_TEST_CHECK(writer.writeUint8(0x01) == Status::no_space);
        CMS_TEST_CHECK(size == 0 && destination == 0xCC);
    }
    for (std::size_t capacity = 0; capacity < 2; ++capacity) {
        std::uint8_t destination[2] = {0xCC, 0xCC};
        std::size_t size = 0;
        BinaryWriter writer(ByteBuffer(
            capacity == 0 ? nullptr : destination, capacity, size));
        CMS_TEST_CHECK(writer.writeUint16BigEndian(0x0102) == Status::no_space);
        CMS_TEST_CHECK(size == 0 && destination[0] == 0xCC && destination[1] == 0xCC);
    }
    for (std::size_t capacity = 0; capacity < 4; ++capacity) {
        std::uint8_t destination[4] = {0xCC, 0xCC, 0xCC, 0xCC};
        std::size_t size = 0;
        BinaryWriter writer(ByteBuffer(
            capacity == 0 ? nullptr : destination, capacity, size));
        CMS_TEST_CHECK(writer.writeUint32BigEndian(0x01020304U) == Status::no_space);
        CMS_TEST_CHECK(size == 0);
        for (std::size_t i = 0; i < 4; ++i) CMS_TEST_CHECK(destination[i] == 0xCC);
    }

    for (std::size_t capacity = 0; capacity < 8; ++capacity) {
        std::uint8_t destination[8] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
        std::size_t size = 0;
        ByteBuffer buffer(capacity == 0 ? nullptr : destination, capacity, size);
        BinaryWriter writer(buffer);
        CMS_TEST_CHECK(writer.writeUint64BigEndian(0x0102030405060708ULL) == Status::no_space);
        CMS_TEST_CHECK(writer.position() == 0);
        CMS_TEST_CHECK(size == 0);
        for (std::size_t i = 0; i < 8; ++i) CMS_TEST_CHECK(destination[i] == 0xCC);
    }

    BinaryReader reader(ByteView(source, 2));
    ByteView output(source + 7, 1);
    CMS_TEST_CHECK(reader.readBytes(3, output) == Status::out_of_range);
    CMS_TEST_CHECK(reader.position() == 0);
    CMS_TEST_CHECK(output.data() == source + 7 && output.size() == 1);

    const std::size_t maximum = (std::numeric_limits<std::size_t>::max)();
    CMS_TEST_CHECK(reader.skip(maximum) == Status::out_of_range);
    CMS_TEST_CHECK(reader.position() == 0);
    CMS_TEST_CHECK(reader.readBytes(maximum, output) == Status::out_of_range);
    CMS_TEST_CHECK(reader.position() == 0);
    CMS_TEST_CHECK(output.data() == source + 7 && output.size() == 1);

    std::uint8_t rollbackBytes[4] = {0x11, 0x22, 0xCC, 0xCC};
    std::size_t rollbackSize = 2;
    BinaryWriter rollbackWriter(ByteBuffer(rollbackBytes, 4, rollbackSize));
    CMS_TEST_CHECK(
        rollbackWriter.writeUint32BigEndian(0x01020304U) == Status::no_space);
    CMS_TEST_CHECK(rollbackWriter.position() == 2 && rollbackSize == 2);
    CMS_TEST_CHECK(rollbackBytes[0] == 0x11 && rollbackBytes[1] == 0x22);
    CMS_TEST_CHECK(rollbackBytes[2] == 0xCC && rollbackBytes[3] == 0xCC);

    const std::uint8_t one = 1;
    CMS_TEST_CHECK(
        rollbackWriter.writeBytes(ByteView(&one, maximum)) == Status::no_space);
    CMS_TEST_CHECK(rollbackWriter.position() == 2);
    CMS_TEST_CHECK(rollbackBytes[0] == 0x11 && rollbackBytes[1] == 0x22);
}

void testOverlapAndCrc() {
    StaticByteBuffer<8> storage;
    BinaryWriter writer(storage.buffer());
    const std::uint8_t initial[] = {1, 2, 3, 4};
    CMS_TEST_REQUIRE(writer.writeBytes(ByteView(initial)) == Status::ok);
    CMS_TEST_REQUIRE(writer.writeBytes(ByteView(storage.data() + 1, 3)) == Status::ok);
    const std::uint8_t expected[] = {1, 2, 3, 4, 2, 3, 4};
    for (std::size_t i = 0; i < 7; ++i) CMS_TEST_CHECK(storage.data()[i] == expected[i]);

    const std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CMS_TEST_CHECK(cms::util::crc32::isoHdlc(ByteView(check)) == 0xCBF43926U);
    CMS_TEST_CHECK(cms::util::crc32::isoHdlc(ByteView()) == 0U);
    cms::util::crc32::IsoHdlc crc;
    crc.update(ByteView(check, 4));
    crc.update(ByteView(check + 4, 5));
    CMS_TEST_CHECK(crc.value() == 0xCBF43926U);
    crc.reset();
    CMS_TEST_CHECK(crc.value() == 0U);
}

} // namespace

int main() {
    testByteStorage();
    testRoundTripAndUnalignedInput();
    testTransactionalFailuresAtEveryBoundary();
    testOverlapAndCrc();
    return cms::test::finish();
}
