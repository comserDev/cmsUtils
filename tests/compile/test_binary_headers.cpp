#include <cms/util/binary_reader.h>
#include <cms/util/binary_writer.h>
#include <cms/util/byte_buffer.h>
#include <cms/util/byte_view.h>
#include <cms/util/crc32.h>
#include <cms/util/static_byte_buffer.h>

int binaryHeadersCompile() {
    cms::util::StaticByteBuffer<8> buffer;
    cms::util::BinaryWriter writer(buffer.buffer());
    return writer.writeUint8(1) == cms::util::Status::ok ? 0 : 1;
}
