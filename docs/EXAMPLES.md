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
