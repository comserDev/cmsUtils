# cmsUtils V2 예제

cmsUtils는 component마다 resource profile이 다르다.

- `StaticString`과 `StaticQueue`는 fixed storage를 사용하는 deterministic component다.
- fixed-capacity `AsyncLogger`의 queue와 message storage도 고정 용량이며 cmsUtils가 제어하는
  heap allocation을 사용하지 않는다. 다만 완성된 Logger의 전체 resource contract는 선택한
  `Clock`, `Sink`, `Mutex`, `Formatter`에도 의존한다. `StdMutex`, `StdFileSink` 같은 host adapter를
  조합한 구성을 strict embedded/deterministic profile이라고 일반화하지 않는다.
- Arduino, FreeRTOS, stdout 같은 기능은 platform adapter로 분리된다.
- `StdQueueAsyncLogger`는 내부 `std::queue`에서 dynamic allocation이 발생할 수 있다.
- `logf`는 libc `snprintf`를 사용하는 opt-in helper다.

Library 전체나 모든 사용 환경이 항상 zero-heap이라고 가정하면 안 된다. 선택한 component의
resource contract를 기준으로 판단한다.

## 1. StaticString과 명시적 오류 처리

`StorageBytes`에는 terminating NUL이 포함된다. 기본 쓰기 API는 공간 부족 시 destination을
바꾸지 않는 transactional operation이다.

```cpp
#include <cms/util/static_string.h>

cms::util::StaticString<32> message;
const auto assigned = message.assign("temperature=");
if (assigned.status == cms::util::Status::ok) {
    const auto appended = message.append("25.0");
    if (appended.status == cms::util::Status::ok) {
        sendToCapi(message.cStr());
    }
}
```

일부 기록이 필요한 경우에만 `assignTruncated()` 또는 `appendTruncated()`를 명시적으로
선택한다.

## 2. StringView split과 parse

Split 결과는 원본 storage를 가리키는 non-owning view다. Parser는 numeric prefix를 허용하므로
전체 token validation에는 `consumed` 길이도 확인한다.

```cpp
#include <cms/util/parse.h>
#include <cms/util/string_ops.h>

cms::util::StringView tokens[2];
const cms::util::StringView input("port:8080");
const std::size_t count = cms::util::string::split(input, ':', tokens);

if (count == 2) {
    const auto port = cms::util::parse::unsignedInteger(tokens[1]);
    if (port.status == cms::util::Status::ok
        && port.consumed == tokens[1].size()) {
        usePort(port.value);
    }
}
```

## 3. UTF-8 validate, count, substring

`StringView` 자체는 valid UTF-8을 보장하지 않는다. UTF-8 알고리즘의 index는 grapheme cluster가
아닌 Unicode code point 기준이다.

```cpp
#include <cms/util/static_string.h>
#include <cms/util/utf8.h>

const cms::util::StringView input(u8"온도: 25도");
if (cms::util::utf8::validate(input) == cms::util::Status::ok) {
    const auto count = cms::util::utf8::count(input);

    cms::util::StaticString<16> prefix;
    const auto copied = cms::util::utf8::substring(input, 0, 2, prefix.buffer());
    if (count.status == cms::util::Status::ok
        && copied.status == cms::util::Status::ok) {
        consume(prefix.view());
    }
}
```

## 4. StaticQueue와 overwrite policy

`push()`는 full일 때 `Status::no_space`를 반환한다. Oldest 교체가 필요한 지점만
`pushOverwrite()`를 사용한다.

```cpp
#include <cms/util/static_queue.h>

cms::util::StaticQueue<int, 2> queue;
queue.push(10);
queue.push(20);

if (queue.push(30) == cms::util::Status::no_space) {
    queue.pushOverwrite(30);
}

if (const int* value = queue.front()) {
    consume(*value);
    queue.pop();
}
```

## 5. SynchronizedQueue

`SynchronizedQueue`는 queue와 mutex를 값으로 소유한다. `front()` pointer를 lock 밖으로
노출하지 않으며 `consumeFront()` callback이 실행되는 동안 lock을 유지한다.

```cpp
#include <cms/util/platform/std_mutex.h>
#include <cms/util/static_queue.h>
#include <cms/util/sync/synchronized_queue.h>

using Queue = cms::util::StaticQueue<int, 8>;
cms::util::sync::SynchronizedQueue<
    Queue,
    cms::util::platform::StdMutex> queue;

queue.push(42);
queue.consumeFront([](const int& value) {
    consume(value);
});
```

`StdMutex`의 내부 resource 특성은 standard library implementation을 따른다. 동기화가 필요 없는
single-thread 구성에는 `sync::NullMutex`를 선택할 수 있다.

## 6. AsyncLogger와 StdoutSink

Fixed-capacity `AsyncLogger`는 queue, clock, formatter, sink를 compile time에 조합한다. Logger는
singleton이 아니며 application이 lifetime을 소유한다.

```cpp
#include <cms/util/log/async_logger.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/platform/system_clock.h>
#include <cms/util/sync/null_mutex.h>

using Logger = cms::util::log::AsyncLogger<
    96,
    8,
    cms::util::platform::SystemClock,
    cms::util::platform::StdoutSink,
    cms::util::sync::NullMutex>;

Logger logger{
    cms::util::platform::SystemClock{},
    cms::util::platform::StdoutSink{}};

if (logger.log(cms::util::log::Level::info, "ready")
    == cms::util::Status::ok) {
    const cms::util::Status output = logger.drainOne();
    // output 상태 확인
}
```

`drainOne()`은 record를 dequeue한 뒤 sink를 호출한다. Output 실패 시 자동 retry나 requeue는
없다.

## 7. logf opt-in convenience

`logf`는 libc `snprintf` semantics를 쓰는 producer-side helper다. Strict deterministic typed
formatter와 같은 resource contract가 아니다.

```cpp
#include <cms/util/log/printf_log.h>

const cms::util::Status status = cms::util::log::logf(
    logger,
    cms::util::log::Level::info,
    "sensor=%u",
    7U);
```

결과가 message capacity에 들어가지 않으면 enqueue하지 않고 `Status::no_space`를 반환한다.

## 8. Runtime level과 ANSI

Runtime level filter는 enqueue 시점에 적용되고, runtime ANSI mode는 drain/format 시점에
적용된다. 필요한 policy를 logger template argument로 선택한다.

```cpp
#include <cms/util/log/level_filter.h>
#include <cms/util/log/runtime_ansi_formatter.h>

using Filter = cms::util::log::RuntimeLevelFilter;
using Formatter = cms::util::log::RuntimeAnsiFormatter;

using RuntimeLogger = cms::util::log::AsyncLogger<
    96,
    8,
    Clock,
    Sink,
    Mutex,
    Formatter,
    Filter>;

RuntimeLogger logger{Clock{}, Sink{}};
logger.setMinLevel(cms::util::log::Level::warning);
logger.setUseColor(true);
```

`Clock`, `Sink`, `Mutex`는 application 환경에 맞는 실제 타입으로 바꾼다. Runtime level filter는
enqueue 시점에 적용되므로 `setMinLevel()` 또는 `setLoggingEnabled()`와 `log()`를 동시에 호출할
때는 caller가 외부 동기화를 제공해야 한다. ANSI mode는 format/drain 시점에 적용되므로
`RuntimeAnsiFormatter::setUseColor()`와 `drainOne()`을 동시에 호출할 때도 외부 동기화가 필요하다.

## 9. StdFileSink

`StdFileSink`는 `FILE`/stdio resource를 사용하는 host opt-in component다.

```cpp
#include <cms/util/platform/std_file_sink.h>

cms::util::platform::StdFileSink sink;
if (sink.open("application.log", cms::util::platform::FileOpenMode::append)
    == cms::util::Status::ok) {
    const cms::util::Status written = sink.write("line\n");
    const cms::util::Status closed = sink.close();
}
```

Open/write/close 오류를 각각 확인한다.

## 10. TeeSink

`TeeSink`는 두 sink를 값으로 소유한다. `FirstSink`가 non-`ok` `Status`를 반환해도
`SecondSink` 호출을 시도하며, 둘 다 `Status`를 반환했다면 첫 non-`ok`을 우선 반환한다.
Exception은 catch하지 않으므로 `FirstSink::write()`가 throw하면 `SecondSink` 호출은 보장되지
않는다. Partial success가 가능하며 이미 발생한 output은 rollback하지 않는다.

```cpp
#include <utility>

#include <cms/util/log/tee_sink.h>
#include <cms/util/platform/std_file_sink.h>
#include <cms/util/platform/stdout_sink.h>

using Output = cms::util::log::TeeSink<
    cms::util::platform::StdoutSink,
    cms::util::platform::StdFileSink>;

cms::util::platform::StdFileSink file;
if (file.open("application.log") == cms::util::Status::ok) {
    Output output{cms::util::platform::StdoutSink{}, std::move(file)};
    const cms::util::Status status = output.write("line\n");
}
```

## 11. Arduino Serial과 UDP

`ArduinoUdpSink`는 UDP 객체를 소유하지 않는다. Application이 WiFi 연결, `udp.begin()`, local
port와 UDP lifetime을 관리한다. Formatted log line 하나가 UDP packet 하나가 된다.

```cpp
WiFiUDP udp;
udp.begin(localPort);

using SerialSink = cms::util::platform::ArduinoSerialSink<HardwareSerial>;
using UdpSink = cms::util::platform::ArduinoUdpSink<WiFiUDP, IPAddress>;
using Output = cms::util::log::TeeSink<SerialSink, UdpSink>;

Output output{
    SerialSink{Serial},
    UdpSink{udp, remoteAddress, remotePort}};
```

전체 logger wiring은 [`examples/ArduinoUdpLogger/ArduinoUdpLogger.ino`](../examples/ArduinoUdpLogger/ArduinoUdpLogger.ino)를 참고한다.

## 12. StdQueueAsyncLogger host opt-in

`StdQueueAsyncLogger`는 `std::queue` storage를 명시적으로 선택하는 host convenience다. Dynamic
allocation이 가능하고 capacity/full/overwrite contract가 없다.

```cpp
#include <cms/util/log/std_queue_async_logger.h>

using HostLogger = cms::util::log::StdQueueAsyncLogger<
    128,
    Clock,
    Sink,
    Mutex>;

HostLogger logger{Clock{}, Sink{}};
```

Allocation과 exception 동작은 underlying standard container와 allocator를 따른다. Fixed
capacity와 predictable storage가 필요하면 `log::AsyncLogger`를 사용한다.

## 13. ByteView와 caller-owned ByteBuffer

`ByteView`는 문자열 의미 없이 pointer와 byte 길이만 참조하므로 embedded NUL을 보존한다.
`ByteBuffer`는 caller가 소유한 storage와 현재 size를 함께 alias한다. Buffer와 size 변수는
`ByteBuffer` 및 이를 사용하는 writer보다 오래 살아야 한다.

```cpp
#include <cstddef>
#include <cstdint>

#include <cms/util/byte_buffer.h>

std::uint8_t storage[32] = {};
std::size_t payloadSize = 0;
cms::util::ByteBuffer payload(storage, sizeof(storage), payloadSize);

// Raw write 뒤 commit에 성공해야 새 size가 publish된다.
payload.data()[0] = 0x41;
payload.data()[1] = 0x00;
payload.data()[2] = 0x42;
if (payload.commit(3) == cms::util::Status::ok) {
    consumeBytes(payload.view()); // 세 byte 모두 전달된다.
}
```

`capacity == 0`, `size == 0`인 buffer는 `data == nullptr`이어도 valid하다. `size > capacity`나
non-zero capacity의 null storage는 invalid binding이다. `commit()` 실패는 기존 size를 바꾸지
않지만 raw access로 caller가 이미 바꾼 byte까지 rollback하지는 않는다.

## 14. StaticByteBuffer와 big-endian writer

`StaticByteBuffer<N>`은 payload storage를 직접 소유하며 최대 `N` byte를 모두 사용할 수 있다.
String과 달리 terminating NUL 자리를 예약하지 않는다. `BinaryWriter`는 생성 시점의 buffer 끝에
이어 쓰고 성공한 operation만 shared size를 전진시킨다.

```cpp
#include <cstdint>

#include <cms/util/binary_writer.h>
#include <cms/util/static_byte_buffer.h>

cms::util::StaticByteBuffer<16> output;
cms::util::BinaryWriter writer(output.buffer());

if (writer.writeUint8(2) != cms::util::Status::ok
    || writer.writeUint16BigEndian(0x1234) != cms::util::Status::ok
    || writer.writeUint32BigEndian(0x89ABCDEFU) != cms::util::Status::ok) {
    // 각 실패 operation은 buffer content, size, writer position을 바꾸지 않는다.
    handleEncodeFailure();
} else {
    sendBytes(output.view());
}
```

`writeUint16BigEndian`, `writeUint32BigEndian`, `writeUint64BigEndian`은 host byte order와
alignment에 의존하지 않는다. `writeBytes()`는 source가 destination storage와 겹쳐도 지원한다.
공간 부족은 `Status::no_space`, invalid output binding은 `Status::invalid_argument`다.

## 15. Transactional big-endian reader

Reader는 source를 소유하지 않으므로 input storage가 reader와 반환된 subview보다 오래 살아야
한다. 입력이 부족하면 output argument와 cursor가 모두 유지된다.

```cpp
#include <cstdint>

#include <cms/util/binary_reader.h>

cms::util::BinaryReader reader(receivedBytes);
std::uint8_t version = 0;
std::uint16_t payloadLength = 0;

if (reader.readUint8(version) != cms::util::Status::ok
    || reader.readUint16BigEndian(payloadLength) != cms::util::Status::ok) {
    rejectIncompleteInput();
} else {
    cms::util::ByteView payload;
    if (reader.readBytes(payloadLength, payload) != cms::util::Status::ok) {
        // readBytes 실패 전 position과 payload 값이 그대로 유지된다.
        waitForMoreData();
    } else if (!reader.empty()) {
        rejectTrailingBytes();
    } else {
        processPayload(version, payload);
    }
}
```

`readBytes()`가 반환한 view는 원본 input을 alias한다. 독립 lifetime이 필요하면 application-owned
storage로 복사한다. `skip()`도 count 전체가 남아 있을 때만 cursor를 전진시킨다.

## 16. CRC-32/ISO-HDLC

한 번에 계산할 때는 `crc32::isoHdlc()`, stream이나 여러 buffer 조각을 순서대로 계산할 때는
`crc32::IsoHdlc`를 사용한다. `value()`는 현재까지 입력한 모든 byte의 finalized checksum을
반환하며 호출 후에도 `update()`를 계속할 수 있다.

```cpp
#include <cstdint>

#include <cms/util/crc32.h>

const std::uint8_t checkBytes[] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9'};

const std::uint32_t oneShot =
    cms::util::crc32::isoHdlc(cms::util::ByteView(checkBytes));
// oneShot == 0xCBF43926

cms::util::crc32::IsoHdlc incremental;
incremental.update(cms::util::ByteView(checkBytes, 4));
incremental.update(cms::util::ByteView(checkBytes + 4, 5));
const std::uint32_t chunked = incremental.value();
// chunked == oneShot
```

이 CRC는 전송 오류 검출용이며 authentication, encryption, collision-resistant hash가 아니다.
Wire format이 checksum field를 0으로 간주하도록 정의한다면 해당 field를 제외한 두 구간 또는
0 byte 네 개를 포함한 구간을 protocol layer에서 올바른 순서로 `update()`한다.

## 17. Binary utility와 protocol의 경계

Binary utility는 unsigned integer와 raw byte sequence만 읽고 쓴다. 다음 의미는 application 또는
protocol codec이 정의한다.

- TLV field ID와 length 규칙
- frame magic, version, header schema
- message ID, correlation, ACK와 retry
- session, authentication, transport reassembly

예를 들어 TLV를 읽을 때 `BinaryReader`로 field ID와 big-endian length를 읽을 수 있지만 unknown
field 처리, duplicate field 거부, required field 확인은 protocol decoder 책임이다. 이렇게 하면
generic utility가 특정 firmware나 wire protocol에 종속되지 않는다.
