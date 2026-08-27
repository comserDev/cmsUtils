# V1에서 V2로 마이그레이션

cmsUtils V2는 V1과 source-compatible한 release가 아니다. V1 compatibility alias도 제공하지
않으므로 subsystem별로 API를 교체해야 한다. V2의 generic public API는 `cms::util` 아래에
있으며, `cms` root에는 `Status` 같은 generic 이름을 추가하지 않는다.

V2는 fixed-capacity 기반의 deterministic/zero-heap component와 `std::queue`, file output 같은
host opt-in component를 구분한다. 기존 V1 코드를 바로 제거하기보다 이 문서를 기준으로 사용처를
차례로 전환하고, 각 단계에서 반환 상태와 resource contract를 확인하는 방식을 권장한다.

## Include와 namespace

대표적인 include와 타입은 다음처럼 바뀐다.

| V1 | V2 |
| --- | --- |
| `#include <cmsString.h>` | `#include <cms/util/static_string.h>` |
| `cms::String<N>` | `cms::util::StaticString<N>` |
| `#include <cmsQueue.h>` | `#include <cms/util/static_queue.h>` |
| `cms::Queue<T, N>` | `cms::util::StaticQueue<T, N>` |
| `cms::ThreadSafeQueue<...>` | `cms::util::sync::SynchronizedQueue<Queue, Mutex>` |
| `#include <cmsAsyncLogger.h>` | `#include <cms/util/log/async_logger.h>` |

Logger는 필요한 policy와 adapter를 명시적으로 include한다. 예를 들어 formatter는
`<cms/util/log/formatter.h>`, host clock과 sink는 각각
`<cms/util/platform/system_clock.h>`, `<cms/util/platform/stdout_sink.h>`에 있다.
V2 타입을 `cms::Status`나 `cms::String`으로 노출하는 root-level convenience alias는 없다.

## String

### 타입과 storage 크기

`cms::String<N>`은 `cms::util::StaticString<N>`으로 교체한다. `StaticString`의
`StorageBytes`에는 terminating NUL이 포함되므로 `StaticString<16>::maxSize()`는 15다.

V1에서 owning `String<N>`의 공통 로직을 담당하던 non-owning `StringBase`는 제거됐다. 외부
또는 fixed storage를 수정하는 알고리즘에는 상속 대신 `StringBuffer` composition을 사용한다.
`StringBuffer`는 문자 storage와 현재 size state를 공유하는 mutable non-owning view이므로,
두 대상의 lifetime을 caller가 보장해야 한다. V1의 `Token` 역할은 `StringView`가 맡는다.

C API에 NUL-terminated 문자열을 전달할 때는 `cStr()`을 사용하고, 길이를 받는 byte API에는
`view()`를 우선 사용한다. `StringView` 기반 API는 embedded NUL도 payload byte로 보존하지만,
`cStr()`을 받는 C API에서는 당연히 C-string 규칙이 적용된다.

### 변경과 truncation

V2의 기본 `assign()`과 `append()`는 transactional하다. 전체 결과가 들어가지 않으면
`Status::no_space`를 반환하고 destination을 바꾸지 않는다. 일부만 기록해야 하는 코드에서는
그 의도를 드러내는 `assignTruncated()` 또는 `appendTruncated()`를 사용한다.

`WriteResult::written`과 `required`는 기존 payload와 terminating NUL을 포함하지 않는다.
따라서 `WriteResult`를 반환하는 쓰기 API에서는 `result.status`를 확인해야 한다.

## String operation 대응

| V1 의도 | V2 API |
| --- | --- |
| `trim()` | `string::trimAsciiWhitespace(view)` |
| equality / ordering | `string::equals`, `string::compare` |
| ASCII ignore-case | `compareIgnoreAsciiCase`, `equalsIgnoreAsciiCase`, `startsWithIgnoreAsciiCase`, `endsWithIgnoreAsciiCase` |
| ignore-case search | `findIgnoreAsciiCase`, `findLastIgnoreAsciiCase` |
| `contains()` | `string::find(...) != string::npos` |
| `split()` | caller-owned `StringView[N]`과 `string::split()` |
| `replace()` | `string::replaceAll()` |
| `byteSubstring()` | `StringView::substr()` |

V1의 `find`, `indexOf`, `lastIndexOf` 사용처가 UTF-8 logical character index를 기대했다면 특히
주의해야 한다. V2의 `string::find()`와 `findLast()`는 **byte index**를 반환한다. Unicode code
point 단위 처리는 `utf8` namespace를 사용한다.

V2 split은 원본을 수정하지 않으며 결과 view는 원본 storage를 가리킨다. V1의 destructive
split은 복원하지 않았다. `copyTokens`나 `splitTo`가 필요했던 코드는 먼저 `StringView` 배열로
나눈 뒤 필요한 token만 `StaticString::assign()`으로 복사한다.

## UTF-8

| V1 기능 | V2 API |
| --- | --- |
| count | `utf8::count()` |
| validation | `utf8::validate()` |
| sanitize | `utf8::sanitize(input, output)` |
| substring | `utf8::substring(input, firstCodePoint, count, output)` |
| 단일 decode | `utf8::decodeNext()` |

`StringView` 자체는 valid UTF-8을 보장하지 않는다. `utf8::substring()`의 index는 Unicode
code point 기준이며 grapheme cluster 기준이 아니다. V2는 Unicode normalization,
locale-aware case folding, grapheme segmentation을 지원한다고 주장하지 않는다.

V2 sanitize는 `StringBuffer` output을 받는 별도 알고리즘이다. V1 in-place sanitize와 같은
signature가 아니며, input과 output storage는 겹치면 안 된다.

## 숫자 format과 parse

| V1 | V2 |
| --- | --- |
| `appendInt`, `fromInt` | `format::signedInteger`, `format::appendSignedInteger` |
| unsigned integer formatting | `format::unsignedInteger`, `format::appendUnsignedInteger` |
| `appendFloat`, `fromFloat` | `format::floatingPoint`, `format::appendFloatingPoint` |
| `toInt`, `hexToInt` | `parse::signedInteger`, `parse::unsignedInteger` |
| `toFloat` | `parse::floatingPoint` |

Integer base는 10과 16을 지원하며 base 16 parser는 optional `0x`/`0X` prefix를 처리한다.
Floating formatter는 precision 0부터 9까지의 fixed decimal만 출력한다. Floating parser도
fixed decimal prefix만 읽으며 exponent, `NaN`/`Inf`, whitespace, locale 표현은 지원하지
않는다. Parser는 double 누적 결과를 반환하며 correctly-rounded decimal conversion을
보장하지 않는다.

V1의 `toInt()`와 `toFloat()`는 ASCII leading whitespace를 허용했다. `isDigit()`, `isHex()`,
`isNumeric()`은 leading과 trailing ASCII whitespace를 모두 허용했다. 반면 V2 `parse::*`는
항상 byte 0부터 시작하며 whitespace를 자동으로 건너뛰지 않는다. 기존 whitespace semantics가
필요한 call site는 parser를 바꾸지 말고 먼저 trim한다.

```cpp
const auto trimmed = cms::util::string::trimAsciiWhitespace(input);
```

전체 입력 validation은 다음처럼 parse 상태와 consumed 길이를 함께 확인한다.

```cpp
const auto digit = cms::util::parse::signedInteger(trimmed, 10);
const bool isDigit = digit.status == cms::util::Status::ok
    && digit.consumed == trimmed.size();

const auto hex = cms::util::parse::unsignedInteger(trimmed, 16);
const bool isHex = hex.status == cms::util::Status::ok
    && hex.consumed == trimmed.size();

const auto numeric = cms::util::parse::floatingPoint(trimmed);
const bool isNumeric = numeric.status == cms::util::Status::ok
    && numeric.consumed == trimmed.size();
```

`toInt()`, `toFloat()`, `hexToInt()` 사용처도 기존 leading whitespace 입력을 계속 받아야 한다면
`trimAsciiWhitespace()` 결과를 각각 `signedInteger()`, `floatingPoint()`,
`unsignedInteger(..., 16)`에 전달한다.

## 일반 String printf의 변경

V1의 `String::printf`, `String::appendPrintf`, `cms::string::appendPrintf`에 해당하는 범용
String formatter는 V2 core에 없다. Deterministic typed formatting과 printf truncation/error
규칙을 분리하고 public API를 작게 유지하기 위한 의도적인 변경이다.

- literal과 view는 `string::append()`를 사용한다.
- integer는 `format::*Integer()`를 사용한다.
- floating-point는 `format::floatingPoint()`를 사용한다.
- Logger producer의 printf-style convenience가 필요하면 `cms::util::log::logf()`를 사용한다.

`logf()`는 libc `snprintf` semantics를 사용하는 opt-in helper다. Strict deterministic
formatter와 같은 resource contract로 간주하면 안 된다. V1 integer width와 `padChar`는 별도
V2 API로 복원하지 않았으므로 필요한 application에서 padding을 명시적으로 조립한다.

## 복원하지 않은 String convenience

| V1 기능 | V2 전환 방식 |
| --- | --- |
| `operator+`, `operator<<` | `assign`/`append`와 typed `format` 호출을 명시적으로 조합 |
| implicit `const char*` | C API에는 `cStr()`, length 기반 API에는 `view()` 사용 |
| mutable `operator[]` | `StringBuffer::data()`로 raw edit 후 `commit(newSize)` 또는 `clear()` 수행 |
| `toUpperCase`, `toLowerCase` | 필요한 application에서 ASCII transform을 명시적으로 구현 |
| `insert`, `remove` | view/UTF-8 substring과 explicit output composition 사용 |
| destructive split | non-destructive `string::split()` 사용 |
| `matches(regex)` | V2 core에는 regex가 없음 |
| `utilization()` | `size()`와 `maxSize()`로 application에서 계산 |
| `peakUtilization()` | core state로 복원하지 않음 |

`StaticString`을 raw edit해야 한다면 먼저 `auto buffer = text.buffer();`로 `StringBuffer`를 얻은
뒤 `buffer.data()`를 수정한다. `StaticString`을 다시 관찰하기 전에 반드시
`buffer.commit(newSize)` 또는 `buffer.clear()`로 invariant를 복구한다.

Regex는 2.0 core 기능이 아니다. Optional 기능으로 추가될 가능성을 현재 제공되는 기능처럼
전제하면 안 된다. Peak profiling도 core object state에 포함하지 않는다.

## Queue

V1 `Queue<T, N>`은 `StaticQueue<T, N>`으로, `ThreadSafeQueue`는
`sync::SynchronizedQueue<Queue, Mutex>` 조합으로 교체한다.

가장 중요한 차이는 full 정책이다. V1 `enqueue()`는 full일 때 oldest를 암묵적으로 덮어썼지만,
V2 `push()`는 `Status::no_space`를 반환하고 FIFO를 보존한다. Overwrite가 정말 필요한 call
site만 `pushOverwrite()`를 사용한다. Logger도 `RejectOnFull`이 기본이며
`OverwriteOldestOnFull`을 명시적으로 선택할 수 있다.

V1 `pop(T&)` 대신 V2에서는 `front()`가 반환한 pointer를 확인한 뒤 `pop()`한다. `front()`는
empty일 때 `nullptr`이며 pointer lifetime은 해당 element의 `pop`/`clear` 또는 queue 파괴
전까지만 유효하다. Thread-safe wrapper는 lock 밖으로 raw pointer를 노출하지 않으므로
`consumeFront()` callback을 사용한다. V1의 `getAt` random access는 의도적으로 제거됐다.

`FreeRtosStaticMutex`는 static allocation을 사용한다. Dynamic mutex fallback은 없으며 ISR-safe
mutex라고 간주하면 안 된다.

## Logger

V2 logger는 `Logger = Queue + Mutex + Clock + Formatter + Sink`의 type composition이다.

- `AsyncLogger<>::instance()` singleton은 제거됐다. Logger lifetime과 ownership은 application이
  관리한다.
- V1 `begin()`은 constructor/type composition과 필요한 runtime setter로 대체한다.
- `d/i/w/e/log` printf method는 `logger.log(Level, StringView)`로 바꾼다.
- printf-style producer가 필요하면 `log::logf(logger, Level, format, ...)`를 사용한다.
- V1 `update()`는 `drainOne()`으로 바뀐다.
- V1 `outputLog()` 역할은 Sink가 맡는다.
- Public queue bypass였던 `pushToQueue()`는 없다.

V1 `handleLog()` virtual hook은 V2 2.0에서 복원하지 않았다. 보안 filter나 transform이 필요하면
application producer 단계에서 `log()` 호출 전에 수행한다. Level filtering은 `LevelFilter`
policy로 처리한다.

Clock은 timestamp source이고 Formatter는 presentation policy다. `SteadyClock`과
`ArduinoMillisClock`은 elapsed-time 계열이며, supported host에서 wall time이 필요하면
`SystemClock`을 사용한다. Fixed UTC offset 표시는 `UtcOffsetFormatter`가 drain 시점에 적용한다.
V2에 runtime global time-mode toggle이 있다고 가정하면 안 된다. Runtime level은
`RuntimeLevelFilter`, runtime color는 runtime ANSI formatter 계열로 선택한다.

## Sink와 output

| 목적 | V2 Sink |
| --- | --- |
| default stdout | `platform::StdoutSink` |
| Arduino Serial | `platform::ArduinoSerialSink<Serial>` |
| Arduino/ESP32 UDP | `platform::ArduinoUdpSink<Udp, Address>` |
| host file | `platform::StdFileSink` |
| 두 destination | `log::TeeSink<FirstSink, SecondSink>` |

Sink contract는 `Status write(StringView)`다. `drainOne()`은 queue lock 밖에서 formatter와 sink를
호출하고 sink의 `Status`를 그대로 반환한다. Record는 sink 호출 전에 dequeue되므로 실패 시
자동 retry/requeue하지 않는다. 즉 한 record의 output은 at-most-once attempt semantics다.

`StdFileSink`는 `FILE`/stdio resource를 사용하는 host opt-in component다. 이 기능 때문에
library 전체가 zero-heap이라고 주장하면 안 된다. `ArduinoUdpSink`는 UDP 객체를 소유하지
않으며 application이 socket 초기화와 lifetime을 담당한다. Formatted line 하나를 UDP packet
하나로 전송한다. `TeeSink`는 `FirstSink`가 non-`ok` `Status`를 반환해도 `SecondSink` 호출을
시도하며, 둘 다 `Status`를 반환했다면 첫 non-`ok`을 우선 반환한다. Exception은 catch하거나
`Status`로 변환하지 않으므로 `FirstSink::write()`가 throw하면 `SecondSink` 호출은 보장되지
않는다. 이미 발생한 output은 rollback하지 않는다.

## std::queue Logger

`log::AsyncLogger`는 fixed-capacity queue를 소유하는 deterministic path다.
`log::StdQueueAsyncLogger`는 dynamic allocation이 가능한 host opt-in path다. 후자는
capacity/full/overwrite contract가 없고 allocation과 exception 동작은 underlying standard
container와 allocator를 따른다.

## Error semantics

V2에서는 실패를 반환값에 숨기지 않는다.

- V1 silent truncation은 `Status`/`WriteResult` 확인으로 바뀐다.
- V1 parse failure의 숫자 `0` 반환은 `ParseResult<T>`로 바뀐다.
- V1에서 숨겨졌던 sink/output 실패는 `Status::io_error`로 전파된다.
- V1의 implicit full overwrite는 explicit `pushOverwrite` 또는 logger policy로 바뀐다.

```cpp
const cms::util::WriteResult written = text.append("payload");
if (written.status != cms::util::Status::ok) {
    // destination은 transactional contract에 따라 그대로다.
}

const auto parsed = cms::util::parse::signedInteger(input);
if (parsed.status != cms::util::Status::ok
    || parsed.consumed != input.size()) {
    // prefix 성공과 전체 입력 validation을 구분한다.
}

const cms::util::Status drained = logger.drainOne();
if (drained == cms::util::Status::io_error) {
    // 이미 dequeue된 record는 자동으로 requeue되지 않는다.
}
```

## 최소 before/after 예제

### A. StaticString append

V1:

```cpp
#include <cmsString.h>

cms::String<16> text("ID:");
text.append("42");
```

V2:

```cpp
#include <cms/util/static_string.h>

cms::util::StaticString<16> text;
if (text.assign("ID:").status != cms::util::Status::ok
    || text.append("42").status != cms::util::Status::ok) {
    // 필요한 오류 처리
}
```

### B. split 후 parse

V1:

```cpp
cms::String<32> input("port:8080");
cms::string::Token tokens[2];
const std::size_t count = input.split(':', tokens, 2);
const int port = cms::String<8>(tokens[1]).toInt();
```

V2:

```cpp
#include <cms/util/parse.h>
#include <cms/util/string_ops.h>

cms::util::StringView tokens[2];
const std::size_t count = cms::util::string::split("port:8080", ':', tokens);
if (count == 2) {
    const auto port = cms::util::parse::unsignedInteger(tokens[1]);
    if (port.status == cms::util::Status::ok
        && port.consumed == tokens[1].size()) {
        // port.value 사용
    }
}
```

### C. StaticQueue overwrite

V1:

```cpp
#include <cmsQueue.h>

cms::Queue<int, 2> queue;
queue.enqueue(1);
queue.enqueue(2);
queue.enqueue(3); // oldest를 암묵적으로 덮어씀
```

V2:

```cpp
#include <cms/util/static_queue.h>

cms::util::StaticQueue<int, 2> queue;
queue.push(1);
queue.push(2);
const cms::util::Status status = queue.pushOverwrite(3); // overwrite 의도를 명시
```

### D. AsyncLogger와 StdoutSink

V1:

```cpp
#include <cmsAsyncLogger.h>

auto& logger = cms::AsyncLogger<>::instance();
logger.begin();
logger.i("ready");
logger.update();
```

V2:

```cpp
#include <cms/util/log/async_logger.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/platform/system_clock.h>
#include <cms/util/sync/null_mutex.h>

using Logger = cms::util::log::AsyncLogger<
    64,
    8,
    cms::util::platform::SystemClock,
    cms::util::platform::StdoutSink,
    cms::util::sync::NullMutex>;

Logger logger{
    cms::util::platform::SystemClock{},
    cms::util::platform::StdoutSink{}};

if (logger.log(cms::util::log::Level::info, "ready")
    == cms::util::Status::ok) {
    const cms::util::Status outputStatus = logger.drainOne();
    // outputStatus 확인
}
```

## 의도적으로 남은 차이

- Singleton: application-owned logger object로 대체했다.
- `handleLog` virtual hook: 제거했으며 producer 단계의 explicit filter/transform으로 대체한다.
- Queue random access: FIFO contract를 작게 유지하기 위해 제거했다.
- Destructive split: 원본을 보존하는 `StringView` split으로 대체했다.
- Generic String printf: typed format API와 logger 전용 `logf`로 분리했다.
- String operator/convenience: explicit mutation과 status 확인으로 대체했다.
- Regex: V2 core에 포함하지 않는다.
- Profiling macro와 peak utilization state: core에서 제거했다.
