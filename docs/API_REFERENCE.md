# cmsUtils 2.0 API Reference

cmsUtils는 embedded와 host 환경에서 함께 사용하는 범용 C++17 utility library다. V2 public
API는 `cms::util` 아래에 있고 canonical public header는 `include/cms/util`에 있다. `cms` root
convenience alias는 제공하지 않는다. V2는 V1과 source-compatible하지 않으며 이전 방법은
[V1 → V2 마이그레이션 가이드](MIGRATION_V1_TO_V2.md)를 참고한다.

`cms::util::detail`과 `cms::util::log::detail`은 구현 전용이므로 직접 사용하지 않는다.

## 1. 공통 결과 타입

Header: `<cms/util/status.h>` · Namespace: `cms::util`

```cpp
enum class Status : std::uint8_t {
    ok, no_space, invalid_argument, invalid_utf8,
    out_of_range, unsupported, io_error
};

struct WriteResult {
    Status status;
    std::size_t written;
    std::size_t required;
};

template<class T>
struct ParseResult {
    Status status;
    T value{};
    std::size_t consumed;
};
```

`written`과 `required`는 이번 operation의 payload byte 수이며 기존 destination 크기와 terminating
NUL을 포함하지 않는다. Transactional API가 실패하면 `written == 0`이고 destination은
불변이다. 명시적 truncation API는 실제 기록량과 전체 필요량을 각각 반환한다.
`ParseResult::consumed`는 input 시작부터 소비한 byte 수다. `T`는 value-initializable이어야 한다.

## 2. 문자열 storage와 view

### StringView

Header: `<cms/util/string_view.h>` · Type: `cms::util::StringView`

```cpp
constexpr StringView() noexcept;
constexpr StringView(const char* data, std::size_t size) noexcept;
template<std::size_t N> constexpr StringView(const char (&array)[N]) noexcept;
constexpr const char* data() const noexcept;
constexpr std::size_t size() const noexcept;
constexpr bool empty() const noexcept;
constexpr char operator[](std::size_t index) const noexcept;
constexpr StringView substr(std::size_t offset, std::size_t count) const noexcept;
```

Read-only byte storage를 가리키는 non-owning view다. NUL termination과 UTF-8 validity를
보장하지 않고 embedded NUL을 허용한다. Caller가 storage lifetime을 보장한다. Pointer와 길이
생성자는 raw byte-view, 배열 생성자는 배열 안의 첫 NUL까지만 보는 bounded C-string semantics다.
`nullptr`은 빈 view로 canonicalize한다. `operator[]`은 `index < size()`가 precondition이다.
`substr()`는 byte offset을 사용하고 count를 clamp하며 범위를 벗어난 offset에는 빈 view를
반환한다. `const char*`, `std::string`, `std::string_view`로의 implicit conversion은 없다.

### StringBuffer

Header: `<cms/util/string_buffer.h>` · Type: `cms::util::StringBuffer`

```cpp
StringBuffer(char* data, std::size_t capacity, std::size_t& size) noexcept;
char* data() noexcept;
const char* data() const noexcept;
std::size_t size() const noexcept;
std::size_t capacity() const noexcept;
std::size_t maxSize() const noexcept;
std::size_t remaining() const noexcept;
bool empty() const noexcept;
bool valid() const noexcept;
StringView view() const noexcept;
Status clear() noexcept;
Status commit(std::size_t newSize) noexcept;
```

Mutable NUL-terminated storage와 size state를 함께 참조하는 non-owning buffer다. 복사본도 같은
storage와 size를 alias하며 caller가 모두의 lifetime을 보장한다. `capacity()`에는 NUL 자리가
포함된다. `data()`로 raw edit한 뒤에는 관찰 전에 `commit(newSize)` 또는 `clear()`로
`size < capacity && data[size] == '\0'` invariant를 복구해야 한다. `commit()`은 payload를
publish하고 NUL을 기록한다. Unbound는 `invalid_argument`, capacity 초과는 `no_space`이며 기존
상태를 바꾸지 않는다.

### StaticString

Header: `<cms/util/static_string.h>` · Type: `cms::util::StaticString<StorageBytes>`

```cpp
const char* cStr() const noexcept;
const char* data() const noexcept;
std::size_t size() const noexcept;
std::size_t capacity() const noexcept;
std::size_t maxSize() const noexcept;
std::size_t remaining() const noexcept;
bool empty() const noexcept;
StringView view() const noexcept;
StringBuffer buffer() noexcept;
void clear() noexcept;
WriteResult assign(StringView value) noexcept;
WriteResult append(StringView value) noexcept;
WriteResult assignTruncated(StringView value) noexcept;
WriteResult appendTruncated(StringView value) noexcept;
```

`StorageBytes`에 terminating NUL이 포함되므로 최대 payload는 `StorageBytes - 1` byte다. Storage를
직접 소유하고 heap을 사용하지 않는다. Copy/move 뒤에도 각 객체는 자신의 storage를 사용하며
move source는 빈 문자열이 된다. 기본 assign/append는 transactional이고 `Truncated` variant만
byte truncation을 허용한다. Truncation은 UTF-8 경계를 보장하지 않는다. `buffer()`는 내부
storage와 size를 alias하므로 원본보다 오래 유지할 수 없다.

## 3. String operations

Header: `<cms/util/string_ops.h>` · Namespace: `cms::util::string`

```cpp
inline constexpr std::size_t npos = static_cast<std::size_t>(-1);
int compare(StringView lhs, StringView rhs) noexcept;
bool equals(StringView lhs, StringView rhs) noexcept;
bool startsWith(StringView value, StringView prefix) noexcept;
bool endsWith(StringView value, StringView suffix) noexcept;
StringView trimAsciiWhitespace(StringView value) noexcept;
int compareIgnoreAsciiCase(StringView lhs, StringView rhs) noexcept;
bool equalsIgnoreAsciiCase(StringView lhs, StringView rhs) noexcept;
bool startsWithIgnoreAsciiCase(StringView value, StringView prefix) noexcept;
bool endsWithIgnoreAsciiCase(StringView value, StringView suffix) noexcept;
std::size_t find(StringView value, StringView needle,
                 std::size_t start = 0) noexcept;
std::size_t findLast(StringView value, StringView needle) noexcept;
std::size_t findIgnoreAsciiCase(StringView value, StringView needle,
                                std::size_t start = 0) noexcept;
std::size_t findLastIgnoreAsciiCase(StringView value,
                                    StringView needle) noexcept;
template<std::size_t N>
std::size_t split(StringView input, char delimiter,
                  StringView (&tokens)[N]) noexcept;
WriteResult copy(StringView input, StringBuffer output) noexcept;
WriteResult copyTruncated(StringView input, StringBuffer output) noexcept;
WriteResult append(StringView input, StringBuffer output) noexcept;
WriteResult appendTruncated(StringView input, StringBuffer output) noexcept;
WriteResult replaceAll(StringView input, StringView needle,
                       StringView replacement, StringBuffer output) noexcept;
```

비교와 검색 index는 byte 기준이다. ASCII ignore-case는 `A-Z`/`a-z`만 fold하며 locale이나
Unicode case folding을 하지 않는다. Trim 대상은 ASCII space, tab, newline, vertical tab,
form feed, carriage return이다.

`split()`은 caller-owned `StringView[N]`을 채우고 token은 원본을 alias한다. Empty field와
trailing empty field를 보존한다. Slot이 부족하면 마지막 slot에 remainder 전체를 넣고
`N == 1`이면 input 전체가 한 token이다. Embedded NUL은 일반 byte다.
Copy/append 계열은 overlap을 지원한다. `replaceAll()` output은 input, needle, replacement와
겹치면 안 된다.

## 4. UTF-8

Header: `<cms/util/utf8.h>` · Namespace: `cms::util::utf8`

```cpp
struct DecodeResult {
    Status status;
    char32_t codePoint;
    std::size_t bytes;
};
DecodeResult decodeNext(StringView input, std::size_t offset) noexcept;
Status validate(StringView input) noexcept;
ParseResult<std::size_t> count(StringView input) noexcept;
WriteResult substring(StringView input, std::size_t firstCodePoint,
                      std::size_t count, StringBuffer output) noexcept;
WriteResult sanitize(StringView input, StringBuffer output) noexcept;
```

Unicode scalar value/code point 기준이며 grapheme cluster, normalization, locale case folding은
지원하지 않는다. `decodeNext()`는 invalid sequence에서 byte 하나를 소비하고 U+FFFD를 반환한다.
`sanitize()`도 invalid byte마다 U+FFFD를 기록한다. `substring()`과 `sanitize()`는
transactional이며 input/output overlap을 지원하지 않는다.

## 5. Number format/parse

### Formatting

Header: `<cms/util/format.h>` · Namespace: `cms::util::format`

```cpp
WriteResult unsignedInteger(std::uint64_t, StringBuffer,
                            unsigned int base = 10,
                            bool uppercase = false) noexcept;
WriteResult signedInteger(std::int64_t, StringBuffer,
                          unsigned int base = 10,
                          bool uppercase = false) noexcept;
WriteResult appendUnsignedInteger(std::uint64_t, StringBuffer,
                                  unsigned int base = 10,
                                  bool uppercase = false) noexcept;
WriteResult appendSignedInteger(std::int64_t, StringBuffer,
                                unsigned int base = 10,
                                bool uppercase = false) noexcept;
WriteResult floatingPoint(double, StringBuffer,
                          unsigned int decimalPlaces = 2) noexcept;
WriteResult appendFloatingPoint(double, StringBuffer,
                                unsigned int decimalPlaces = 2) noexcept;
```

Integer base는 10/16만 지원하고 numeric prefix를 출력하지 않는다. 기본 함수는 output을 교체하고
append variant는 기존 payload 뒤에 붙인다. Float는 scientific notation 없는 fixed decimal이며
precision 0..9를 지원한다. Represented `double` 기준 nearest, exact halfway away from zero로
반올림하고 negative zero를 보존한다. NaN/Inf와 잘못된 precision은 `invalid_argument`, 안전한
integer magnitude 범위를 넘으면 `out_of_range`다. 모든 operation은 heap 없이 transactional하다.

### Parsing

Header: `<cms/util/parse.h>` · Namespace: `cms::util::parse`

```cpp
ParseResult<std::uint64_t> unsignedInteger(
    StringView input, unsigned int base = 10) noexcept;
ParseResult<std::int64_t> signedInteger(
    StringView input, unsigned int base = 10) noexcept;
ParseResult<double> floatingPoint(StringView input) noexcept;
```

Byte 0부터 시작하며 whitespace를 skip하지 않는다. 성공은 numeric prefix만 소비해도 되므로 full
validation에는 `consumed == input.size()`를 함께 검사한다. Integer base는 10/16만 지원한다.
Unsigned는 sign을 거부하고 signed는 leading sign 하나를 허용한다. Base 16의 optional
`0x`/`0X` 뒤에는 hex digit이 필요하다. Overflow는 `out_of_range`, value 0, 처음 overflow를
확정한 digit offset을 반환한다.

Float grammar는 optional sign 뒤 `digits`, `digits.`, `digits.fraction`, `.fraction`이다.
Exponent, whitespace, NaN/Inf, locale 표현은 지원하지 않는다. Embedded NUL은 prefix를 끝내는
non-numeric byte다. Negative zero를 보존하고 긴 fraction은 정상 처리하지만 correctly-rounded
decimal conversion을 보장하지 않는다.

## 6. Containers와 synchronization

### StaticQueue

Header: `<cms/util/static_queue.h>` · Type: `cms::util::StaticQueue<T, Capacity>`

```cpp
std::size_t size() const noexcept;
constexpr std::size_t capacity() const noexcept;
bool empty() const noexcept;
bool full() const noexcept;
T* front() noexcept;
const T* front() const noexcept;
Status push(const T& value);
Status push(T&& value);
template<class... Args> Status emplace(Args&&... args);
Status pushOverwrite(const T& value) noexcept;
Status pushOverwrite(T&& value) noexcept;
Status pop() noexcept;
void clear() noexcept;
```

Fixed-capacity zero-heap FIFO다. `Capacity > 0`, nothrow-destructible `T`가 필요하고 copy/move는
삭제돼 있다. 빈 queue의 `front()`는 `nullptr`이다. Pointer는 해당 element가 제거, 파괴 또는
교체되기 전까지만 유효하다. `pop()`, `clear()`, queue destruction뿐 아니라 full 상태의
`pushOverwrite()`가 oldest/front를 교체해도 기존 front pointer는 무효화된다. 일반
`push()`/`emplace()` 성공은 기존 element를 이동시키지 않는다. Full `push()`/`emplace()`는
FIFO를 보존하고 `no_space`다. `pushOverwrite()`만 oldest를 교체한다. Source가 full queue의 oldest 자체면
`invalid_argument`이며 queue는 불변이다. `back()`과 `getAt()`은 없다.

### Synchronization

Headers와 public types:

- `<cms/util/sync/lock_guard.h>` — `cms::util::sync::LockGuard<Mutex>`
- `<cms/util/sync/null_mutex.h>` — `cms::util::sync::NullMutex`
- `<cms/util/sync/mutex_ref.h>` — `cms::util::sync::MutexRef<Mutex>`
- `<cms/util/sync/synchronized_queue.h>` — `cms::util::sync::SynchronizedQueue<Queue, Mutex>`

`LockGuard`와 `MutexRef`는 mutex를 소유하지 않으므로 참조 대상이 더 오래 살아야 한다.
`NullMutex`는 no-op backend다. `SynchronizedQueue`는 Queue와 Mutex를 값으로 소유하며 copy/move를
금지한다. Mutable queue operation은 lock 안에서 실행한다. Immutable capacity contract에 따라
`capacity()`만 lock-free다. `consumeFront()`는 callback과 pop까지 lock을 유지하고 raw
queue/reference를 밖으로 노출하지 않는다. Callback에서 reference를 보관하거나 장시간
blocking/I/O를 수행하면 안 된다.

## 7. Logging

Namespace: `cms::util::log`

### Records, formatter, policy

```cpp
enum class Level : std::uint8_t {
    trace, debug, info, warning, error, critical
};
using Timestamp = std::uint64_t;
struct Record { Level level; Timestamp timestampMilliseconds; StringView message; };
template<std::size_t MessageBytes> class StaticRecord;
```

Headers: `<cms/util/log/level.h>`, `<cms/util/log/clock.h>`,
`<cms/util/log/record.h>`. `Record`는 message를 소유하지 않는 view이고 `StaticRecord`는 fixed
message storage를 소유한다. `MessageBytes`에는 NUL 자리가 포함된다. Timestamp 단위는
milliseconds이고 epoch는 Clock backend가 정한다.

Formatter headers와 public API:

- `<cms/util/log/formatter.h>` — `levelName`, `format`, `PlainFormatter`
- `<cms/util/log/ansi_formatter.h>` — `formatAnsi`, `AnsiFormatter`
- `<cms/util/log/runtime_ansi_formatter.h>` — `RuntimeAnsiFormatter`
- `<cms/util/log/styled_ansi_formatter.h>` — `formatStyledAnsi`, `StyledAnsiFormatter`,
  `RuntimeStyledAnsiFormatter`
- `<cms/util/log/utc_offset_formatter.h>` — `formatUtcOffsetTimestamp`,
  `UtcOffsetFormatter<Formatter>`

Formatter는 `WriteResult format(const Record&, StringBuffer)` 형태로 한 line을 transactional하게
기록한다. Runtime formatter setter와 format을 동시에 호출하려면 caller synchronization이
필요하다. `UtcOffsetFormatter` 기본 offset은 0, 범위는 -720..+840분이다. 자동 timezone 탐지,
IANA zone, DST 계산은 하지 않으며 offset은 drain/format 시점에 적용된다.

`<cms/util/log/level_filter.h>`는 `NoLevelFilter`, `RuntimeLevelFilter`를 제공한다. Runtime filter
setter와 `log()`의 동시 호출에는 caller synchronization이 필요하다.
`<cms/util/log/full_queue_policy.h>`는 기본 `RejectOnFull`과 opt-in
`OverwriteOldestOnFull`을 제공한다.

### AsyncLogger

Header: `<cms/util/log/async_logger.h>`

```cpp
template<std::size_t MessageBytes, std::size_t QueueCapacity,
         class Clock, class Sink, class Mutex,
         class Formatter = PlainFormatter,
         class LevelFilter = NoLevelFilter,
         class FullQueuePolicy = RejectOnFull>
using AsyncLogger = /* implementation-defined */;
```

주요 API는 `messageCapacity()`, `log`, `drainOne`, `wouldLog`, `pending`, `empty`, `capacity`,
`full`이다. `messageCapacity()`는 fixed-capacity와 `std::queue` logger가 공유하는 message storage
계약이다. 선택한 runtime policy에만 다음 API가 노출된다.

- Runtime ANSI: `setUseColor(bool)`, `useColor()`
- UTC offset: `setUtcOffsetMinutes(int)`, `utcOffsetMinutes()`
- Runtime level: `setMinLevel(Level)`, `minLevel()`, `setLoggingEnabled(bool)`,
  `loggingEnabled()`

Producer는 filter → clock → owning `StaticRecord` copy → enqueue 순서다. Filtered log는
`Status::ok`이고 clock/queue/message-size 검사를 건너뛴다. Input view는 즉시 record에 복사된다.
Consumer는 record copy/dequeue under lock → unlock → formatter → sink 순서다. Formatter/sink는
queue lock 밖에서 실행한다. `drainOne()`은 sink Status를 반환하지만 record는 이미 dequeue됐고
retry/requeue하지 않으므로 at-most-once attempt다. Singleton은 없고 application이 lifetime을
소유한다.

Full queue에서 기본 `RejectOnFull`은 `no_space`와 FIFO 불변을 보장한다.
`OverwriteOldestOnFull`만 oldest를 교체한다.

### StdQueueAsyncLogger와 logf

Header: `<cms/util/log/std_queue_async_logger.h>`

```cpp
template<std::size_t MessageBytes, class Clock, class Sink, class Mutex,
         class Formatter = PlainFormatter,
         class LevelFilter = NoLevelFilter>
using StdQueueAsyncLogger = /* implementation-defined */;
```

`std::queue` storage를 선택하는 host convenience다. Dynamic allocation이 가능하고 fixed
capacity/full/overwrite contract가 없다. Exception과 allocation은 underlying container와
allocator contract를 따르며 `Status`로 변환하지 않는다.

`<cms/util/log/printf_log.h>`의
`logf(Logger&, Level, const char*, Args&&...)`는 libc `snprintf`를 사용하는 opt-in helper다.
Filtered log는 formatting을 건너뛴다. Null/formatting 오류는 `invalid_argument`, oversize는
`no_space`이며 enqueue하지 않는다. Typed formatter와 같은 resource contract라고 주장하지 않는다.

### TeeSink

Header: `<cms/util/log/tee_sink.h>` · Type: `TeeSink<FirstSink, SecondSink>`

두 sink를 값으로 소유한다. First가 non-`ok`이어도 Second를 호출하며 둘 다 반환했다면 첫
non-`ok`을 우선한다. Exception은 catch하지 않으므로 First가 throw하면 Second 호출은 보장되지
않는다. Partial success가 가능하고 rollback하지 않는다.

## 8. Platform adapters

Namespace: `cms::util::platform`

### Mutex와 clock

- `<cms/util/platform/std_mutex.h>` — `StdMutex`: `std::mutex`를 소유하는 host adapter
- `<cms/util/platform/freertos_static_mutex.h>` — `FreeRtosStaticMutex`: static FreeRTOS mutex
- `<cms/util/platform/steady_clock.h>` — `SteadyClock`
- `<cms/util/platform/arduino_millis_clock.h>` — `ArduinoMillisClock`
- `<cms/util/platform/system_clock.h>` — `SystemClock`

`FreeRtosStaticMutex`는 dynamic fallback 없이 `xSemaphoreCreateMutexStatic()`만 쓰며 일반 task
context용 blocking mutex다. ISR-safe API가 아니다. 생성 실패는 fail-stop하고 정상 lock return은
실제 acquisition을 뜻한다.

Clock은 `Timestamp nowMilliseconds()`를 제공한다. `SteadyClock`은 monotonic relative 계열이고
wall clock이 아니다. `ArduinoMillisClock`은 `millis()`를 widen할 뿐 rollover를 연장하지 않는다.
`SystemClock`은 `system_clock` epoch가 Unix epoch인 지원 platform/toolchain용 wall-clock
adapter이고 timezone을 적용하지 않는다. C++17 자체가 모든 implementation의 Unix epoch를
보장하지는 않는다.

### Byte sinks

`<cms/util/platform/stdout_sink.h>`의 `StdoutSink`와
`<cms/util/platform/arduino_serial_sink.h>`의 `ArduinoSerialSink<Serial>`은
`Status write(StringView)`를 제공한다. Empty는 I/O 없이 성공하고 length 기반 exact write로
embedded NUL을 보존한다. Short write는 `io_error`다. Arduino adapter는 Serial-like 객체를
non-owning으로 참조한다.

### StdFileSink

Header: `<cms/util/platform/std_file_sink.h>`

```cpp
enum class FileOpenMode { append, truncate };
Status open(const char* path, FileOpenMode mode = FileOpenMode::append);
Status write(StringView data);
Status flush();
Status close();
bool isOpen() const noexcept;
```

`FILE*`을 소유하는 host stdio adapter다. Copy와 move assignment는 삭제되고 noexcept move
construction만 지원한다. Open된 sink 재open, closed write/flush는 `invalid_argument`다.
Already-closed `close()`는 성공한다. Append는 `ab`, truncate는 `wb`, invalid enum은
`invalid_argument`다. Embedded NUL을 exact bytes로 쓰며 short write/flush/close failure는
`io_error`다. Write 성공은 durability를 보장하지 않는다.

### ArduinoUdpSink

Header: `<cms/util/platform/arduino_udp_sink.h>`

```cpp
template<class Udp, class Address>
class ArduinoUdpSink {
public:
    ArduinoUdpSink(Udp& udp, Address remoteAddress,
                   std::uint16_t remotePort);
    Status write(StringView data);
};
```

UDP-like 객체는 non-owning이고 address는 값으로 소유한다. Application이 initialization,
begin/stop, local port, lifetime을 관리한다. Empty write는 UDP API를 호출하지 않는다. Non-empty
write 하나가 packet 하나이며 beginPacket, exact write, endPacket 중 하나라도 실패하면
`io_error`다. Short write에서도 endPacket을 호출하고 side effect를 rollback하지 않는다.

## 9. Resource profile

### Deterministic / fixed-storage

`StringView`, `StringBuffer`, `StaticString`, string/UTF-8/format/parse algorithm과
`StaticQueue`는 runtime heap을 사용하지 않는다. `SynchronizedQueue` wrapper 자체도 dynamic
allocation을 추가하지 않지만 전체 instance의 resource contract는 Queue와 Mutex backend에
의존한다. `StaticQueue`와 deterministic mutex backend 조합은 deterministic path이고,
`StdMutex` 같은 platform/host backend를 선택하면 해당 backend contract도 함께 적용된다.
Fixed-capacity `AsyncLogger`의 queue/message storage는 고정 용량이지만 완성된 logger의 전체
contract는 선택한 Clock/Sink/Mutex/Formatter에도 의존한다.

### Host/platform-dependent

`StdMutex`, host clock, `StdoutSink`, `StdFileSink`는 host library/stdio resource에 의존한다.
Arduino/FreeRTOS adapter는 해당 platform API와 caller-managed lifecycle을 따른다.

### Dynamic/optional

`StdQueueAsyncLogger`는 `std::queue` allocation을 허용하는 opt-in이다. `logf`는 libc
`snprintf` opt-in helper다. 따라서 cmsUtils 전체가 항상 zero-heap이라고 일반화하지 않는다.

## 10. Header dependency 경계

Generic/deterministic header는 host/platform header에 의존하지 않는다. Host dependency는
`std_mutex.h`, host clock/sink header, `printf_log.h`, `std_queue_async_logger.h`에 격리된다.
Arduino/FreeRTOS dependency도 해당 adapter header에만 있다. `detail` header는 public include
tree에 존재하더라도 implementation 전용이며 public API나 사용자 코드에서 직접 참조하지 않는다.
