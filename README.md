# cmsUtils

`cmsUtils`는 embedded와 host 환경에서 함께 사용할 수 있는 C++17 utility library다.
Fixed-capacity 기반의 deterministic / zero-heap 경로와 dynamic resource를 사용하는 host
convenience 경로를 명시적으로 분리한다.

## 주요 기능

- `StringView`, `StringBuffer`, `StaticString<N>`과 UTF-8/format/parse 알고리즘
- `ByteView`, `ByteBuffer`, `StaticByteBuffer<N>`
- bounds-checked big-endian `BinaryReader`와 `BinaryWriter`
- one-shot/incremental CRC-32/ISO-HDLC
- fixed-capacity `StaticQueue`와 synchronization wrapper
- queue, clock, formatter, sink를 조합하는 `AsyncLogger`
- `StdQueueAsyncLogger`, `StdFileSink` 같은 명시적 host opt-in component
- Arduino, FreeRTOS, standard-library platform adapter

Generic V2 API는 `cms::util` namespace와 `include/cms/util` public header 아래에 있다.
V1의 `cms::String`, `cms::Queue`, singleton logger와 source-compatible하지 않다.

## 빠른 시작

### Fixed-capacity string

```cpp
#include <cms/util/static_string.h>

cms::util::StaticString<32> message;
if (message.assign("temperature=").status == cms::util::Status::ok) {
    (void)message.append("25.0");
}
```

`StaticString<32>`의 32 bytes에는 terminating NUL이 포함된다. 기본 assign/append는 전체
결과가 들어가지 않으면 destination을 변경하지 않는 transactional operation이다.

### Big-endian binary encoding과 CRC

```cpp
#include <cstdint>

#include <cms/util/binary_reader.h>
#include <cms/util/binary_writer.h>
#include <cms/util/crc32.h>
#include <cms/util/static_byte_buffer.h>

cms::util::StaticByteBuffer<16> frame;
cms::util::BinaryWriter writer(frame.buffer());

if (writer.writeUint16BigEndian(0x1234) == cms::util::Status::ok
    && writer.writeUint32BigEndian(0x89ABCDEFU) == cms::util::Status::ok) {
    const std::uint32_t checksum = cms::util::crc32::isoHdlc(frame.view());
    // frame.view()와 checksum 사용
}
```

Binary API는 byte 단위로 조합하므로 host endian과 alignment에 의존하지 않는다. 공간이나 입력이
부족하면 buffer와 cursor를 변경하지 않는다. TLV, frame schema, message ID, session, ACK 같은
protocol 의미는 application에 남긴다.

### Fixed-capacity queue

```cpp
#include <cms/util/static_queue.h>

cms::util::StaticQueue<int, 4> queue;
if (queue.push(42) == cms::util::Status::ok) {
    if (const int* value = queue.front()) {
        consume(*value);
        (void)queue.pop();
    }
}
```

Full queue의 기본 동작은 `Status::no_space`이며 FIFO는 바뀌지 않는다. Oldest 교체가 필요한
경우에만 `pushOverwrite()`를 명시적으로 사용한다.

## Resource contract

Deterministic component는 runtime heap allocation이나 host-only dependency를 사용하지 않는다.
여기에는 fixed byte/string/queue storage, binary reader/writer, CRC와 generic 알고리즘이 포함된다.

`StdQueueAsyncLogger`, `StdMutex`, `StdFileSink`, `SystemClock` 같은 host/platform component는
선택한 backend의 allocation, exception, OS/runtime contract를 따른다. 따라서 library 전체가
항상 zero-heap이라고 일반화하지 않는다.

## 설치와 빌드

### CMake

```cmake
add_subdirectory(cmsUtils)
target_link_libraries(my_target PRIVATE cms::utils)
```

설치된 package를 사용할 때도 `find_package(cmsUtils CONFIG REQUIRED)` 뒤 같은 target을 link한다.

### PlatformIO / ESP32

```ini
lib_deps =
    https://github.com/comserDev/cmsUtils.git

build_unflags =
    -std=gnu++11
    -std=c++11

build_flags =
    -std=gnu++17
    -finput-charset=UTF-8
    -fexec-charset=UTF-8
```

CI는 GCC, Clang, MSVC와 ESP32 Xtensa GCC 8.4 compile path를 확인한다.

## 문서

- [API reference](docs/API_REFERENCE.md)
- [사용 예제](docs/EXAMPLES.md)
- [Binary utilities 계약](docs/BINARY_UTILITIES.md)
- [V1에서 V2로 마이그레이션](docs/MIGRATION_V1_TO_V2.md)

## 라이선스

MIT License

Maintainer: comser.dev

Repository: <https://github.com/comserDev/cmsUtils>
