# cmsUtils 개발 규칙

## 프로젝트 방향

`cmsUtils`는 embedded와 host 환경에서 함께 사용할 수 있는 범용 C++ utility library다.

이 프로젝트는 특정 실행 환경 하나에 종속되지 않는다.

다음 두 사용 경로를 함께 지원하는 것을 목표로 한다.

```text
1. deterministic / zero-heap path
   - fixed-capacity storage
   - predictable memory usage
   - embedded / long-running process에 적합

2. host / convenience path
   - 표준 라이브러리와 host 기능을 필요에 따라 opt-in
   - std::queue, file output 등 dynamic resource 사용 가능
```

embedded는 라이브러리 전체의 정체성이 아니라 하나의 resource profile이다.

설계 우선순위:

1. Correctness
2. Memory Safety
3. Resource Contract Clarity
4. Deterministic / Zero-Heap Path Preservation
5. API Clarity
6. Portability
7. Small RAM Footprint
8. Code Size
9. Testability
10. Performance

성능, RAM, flash, allocation 절감 효과는 실제 측정 없이 주장하지 않는다.

---

## Resource contract

각 public component는 자신의 resource contract를 명확히 가진다.

다음 중 어디에 속하는지 구현과 문서에서 구분한다.

```text
deterministic / zero-heap component
host / dynamic-resource component
platform adapter
optional feature
```

### Deterministic / zero-heap component

다음 원칙을 지킨다.

- runtime dynamic allocation을 사용하지 않는다.
- fixed-capacity 또는 caller-provided storage를 우선한다.
- memory usage가 입력과 무관하게 예측 가능해야 한다.
- hidden allocation을 만들지 않는다.
- host-only dependency를 transitively 끌어오지 않는다.
- 장시간 동작에서도 heap fragmentation에 의존하지 않는다.

대표 예:

```text
StringView
StringBuffer
StaticString
StaticQueue
SynchronizedQueue
StaticQueue 기반 logger configuration
```

### Host / convenience component

목적상 필요한 경우 dynamic resource를 사용할 수 있다.

예:

```text
std::queue 기반 logger configuration
file output
host filesystem adapter
```

단 다음을 지킨다.

- dynamic allocation 사용 여부를 숨기지 않는다.
- 해당 기능을 선택하지 않은 code path에 비용이 유입되지 않게 한다.
- deterministic component가 host convenience에 의존하지 않게 한다.
- standard container가 가질 수 있는 allocation/exception 특성을 거짓으로 숨기지 않는다.

---

## Dynamic allocation 규칙

deterministic / zero-heap production code에서는 다음을 사용하지 않는다.

```text
new            // dynamic allocation 용도
delete
malloc
calloc
realloc
free

std::string
std::vector
std::deque
std::queue
std::list
std::function
std::shared_ptr
std::unique_ptr
```

단, placement new는 fixed raw storage에서 object lifetime을 시작하기 위한 용도로 허용한다.

예:

```cpp
::new (address) T(args...);
```

host / optional component에서는 위 타입을 사용할 수 있지만 다음 조건을 만족해야 한다.

- 사용 이유가 기능과 직접 관련되어야 한다.
- 해당 dependency가 portable/deterministic header로 전파되지 않아야 한다.
- allocation/resource behavior를 public contract에서 명확히 한다.

---

## Portability

범용 알고리즘과 deterministic component는 OS, RTOS, 특정 MCU framework에 종속되지 않는다.

- generic code에서 Windows, POSIX, Arduino, FreeRTOS 등에 직접 의존하지 않는다.
- platform-specific 기능은 adapter 계층으로 분리한다.
- `<mutex>`, `<thread>`, `<chrono>`, filesystem 등의 dependency는 필요한 platform/host layer에서만 사용한다.
- exception과 RTTI를 기본 error handling/dispatch 구조로 사용하지 않는다.
- bare-metal 또는 축소된 C++ runtime에서도 사용할 수 있는 component는 그 portability를 유지한다.
- host-only 기능 때문에 portable public header가 host runtime dependency를 요구하게 만들지 않는다.

예:

```text
StaticQueue
StringView
utf8
    ↓ host dependency 없음

StdMutex
SystemClock
StdFileSink
    ↓ host dependency 허용
```

---

## Dependency direction

dependency는 다음 방향을 우선한다.

```text
platform / host / optional
        ↓
generic utility
        ↓
low-level result / view / buffer
```

다음을 피한다.

```text
generic utility
        ↓
host-only adapter
```

규칙:

- generic algorithm은 platform adapter에 의존하지 않는다.
- deterministic component는 host-only component에 의존하지 않는다.
- platform adapter가 generic utility를 사용하는 방향은 허용한다.
- optional feature 때문에 unrelated public header가 무거운 standard header를 include하지 않게 한다.
- compile-time convenience를 위해 dependency isolation을 깨지 않는다.

---

## C++ 기준

- 최소 language standard는 C++17로 유지한다.
- C++17 선택은 embedded toolchain과 범용 compiler portability를 함께 고려한 것이다.
- 최신 compiler 사용과 minimum language standard 상승을 구분한다.
- C++20/23 기능은 단순 문법 편의를 위해 최소 요구사항을 높이는 용도로 사용하지 않는다.
- 상위 표준 전환은 지원 대상 compiler/toolchain을 먼저 검증한다.

---

## Namespace

프로젝트 root namespace는 `cms`다.

`cmsUtils`의 generic public API는 `cms::util` 아래에 둔다.

예:

```text
cms::util::Status
cms::util::WriteResult
cms::util::ParseResult<T>

cms::util::StringView
cms::util::StringBuffer
cms::util::StaticString<N>
cms::util::StaticQueue<T, N>

cms::util::utf8
cms::util::sync
cms::util::log
cms::util::platform
cms::util::optional
```

규칙:

- `cms` root에 generic 이름을 직접 추가하지 않는다.
- `Status`, `Result`, `Queue`, `String`, `Config`, `Error` 같은 범용 이름을 `cms` root에서 차지하지 않는다.
- `cms::Status` 같은 root-level compatibility alias는 특별한 migration 근거가 없으면 만들지 않는다.
- repository 이름이 `cmsUtils`라고 해서 namespace를 `cms::utils`로 강제하지 않는다.
- 하위 namespace는 lower-case로 작성한다.

사용자 application은 자유롭게 다음과 같이 사용할 수 있어야 한다.

```cpp
namespace cms {

enum class Status {
    booting,
    ready,
    running,
    error
};

}
```

이 경우 library의 `cms::util::Status`와 충돌하지 않아야 한다.

---

## Public header layout

향후 public header는 다음 구조를 기본으로 한다.

```text
include/cms/util/
```

예:

```text
include/cms/util/status.h
include/cms/util/string_view.h
include/cms/util/string_buffer.h
include/cms/util/static_string.h
include/cms/util/static_queue.h

include/cms/util/utf8.h

include/cms/util/sync/lock_guard.h
include/cms/util/sync/null_mutex.h
include/cms/util/sync/mutex_ref.h
include/cms/util/sync/synchronized_queue.h

include/cms/util/log/async_logger.h

include/cms/util/platform/system_clock.h
```

실제 migration은 한 Step에서 일관되게 수행한다.

중간 상태에서 old/new path가 무질서하게 섞이지 않게 한다.

---

## Naming

- 파일명: `lower_snake_case`
- header 확장자: `.h`
- 타입명: `PascalCase`
- 함수 및 메서드: `lowerCamelCase`
- 지역 변수: `lowerCamelCase`
- private member: `trailing underscore`
- macro: `UPPER_SNAKE_CASE`

예:

```cpp
class StaticQueue {
private:
    std::size_t size_;
    std::size_t head_;
};
```

다음 leading underscore 스타일은 사용하지 않는다.

```cpp
_size
_head
_data
```

---

## 주석

- 모든 신규 또는 수정 주석은 UTF-8로 작성한다.
- 자연스러운 한국어를 기본으로 한다.
- 존댓말을 사용하지 않는다.
- 개발자가 코드에 직접 작성한 것 같은 짧고 자연스러운 문체를 사용한다.
- 전문용어, C++ 용어, API 이름은 필요한 경우 영어를 그대로 사용한다.
- 모든 기술 용어를 억지로 번역하지 않는다.
- 코드만 읽어도 명백한 내용을 반복 설명하지 않는다.
- 지나치게 긴 주석을 피한다.

주석은 주로 다음을 설명한다.

- 왜 이런 구현 방식을 사용했는가
- 중요한 contract가 무엇인가
- ownership/lifetime은 어떻게 되는가
- memory/resource behavior는 무엇인가
- overflow, aliasing, object lifetime에서 어떤 caveat가 있는가
- platform/host-only 제한이 있는가

좋은 예:

```cpp
// full 상태에서 source가 oldest와 같으면 lifetime을 끝내기 전에 거부한다.

// offset은 record가 아니라 formatter state이므로 drain 시점의 값을 사용한다.

// host adapter만 system_clock에 의존하며 core timestamp 의미는 바꾸지 않는다.

// lock을 유지하는 동안에만 front reference가 유효하다.
```

피해야 할 예:

```cpp
// 이 함수는 큐가 가득 차 있는지 확인한 다음 큐가 가득 차 있으면...
```

주석 때문에 encoding warning이 발생하지 않도록 파일은 UTF-8을 유지한다.

기존 V1 파일의 encoding 문제를 이유로 파일 전체를 일괄 변환하지 않는다.

---

## Template 사용

- compile-time polymorphism이 적합한 경우 template을 사용한다.
- runtime polymorphism이 필요하지 않은 곳에서 virtual interface를 만들지 않는다.
- vtable과 type erasure를 불필요하게 추가하지 않는다.
- `std::function` 대신 callback template을 우선한다.
- template axis를 추가하기 전에 실제 public value가 있는지 검토한다.
- policy 조합이 과도하게 늘어나 API를 이해하기 어렵게 만들지 않는다.
- template 사용으로 flash/code size가 과도하게 증가할 가능성을 고려한다.
- 측정 없이 template이 항상 zero-cost라고 주장하지 않는다.

---

## Public API

- public API는 작고 명확하게 유지한다.
- 새로운 public type이나 함수는 실제 필요성이 있을 때만 추가한다.
- public API 추가 전 ownership, lifetime, error semantics, resource contract를 먼저 정의한다.
- raw pointer/reference를 반환할 경우 lifetime contract를 명확히 한다.
- synchronization 보호가 끝난 뒤 위험해지는 pointer/reference를 public API로 노출하지 않는다.
- 동일한 의미의 convenience API를 불필요하게 중복하지 않는다.
- 새로운 enum/result type을 남발하지 않고 기존 `cms::util` result type을 우선 재사용한다.

---

## 표준 C++ API 친화성

표준 C++에 동일한 개념과 널리 알려진 API 이름이 있으면 이를 우선 참고한다.

특히 container-like API는 가능한 범위에서 표준 vocabulary를 따른다.

예:

```text
empty()
size()
front()
back()
push()
emplace()
pop()
```

규칙:

- 의미가 같은 operation에 불필요한 custom 이름을 만들지 않는다.
- `enqueue/dequeue`, `tryPush/tryPop` 같은 별도 vocabulary는 실제로 다른 semantics가 있을 때만 사용한다.
- 표준 API와 동일하게 보이기 위해 memory safety를 희생하지 않는다.
- 표준 API와 동일하게 보이기 위해 explicit error semantics를 제거하지 않는다.
- fixed-capacity 특유 기능은 extension임을 이름에서 명확히 한다.

예:

```text
full()
capacity()
pushOverwrite()
```

- `std::queue` 전체 API를 모두 구현할 필요는 없다.
- 실제 지원하는 기능 범위에서 familiarity를 높이는 것이 목적이다.

---

## Error 처리

generic/deterministic component에서는 exception을 기본 error handling 방식으로 사용하지 않는다.

가능하면 다음 명시적 result type을 재사용한다.

```text
cms::util::Status
cms::util::WriteResult
cms::util::ParseResult<T>
```

규칙:

- 실패 시 destination을 변경하지 않는 transactional semantics를 우선한다.
- truncation은 명시적으로 요청된 API에서만 허용한다.
- overflow 가능한 연산은 계산 전에 검증한다.
- invalid argument를 silent clamp/wrap으로 숨기지 않는다.
- host standard component가 exception을 발생시킬 수 있다면 그 특성을 documentation/noexcept contract에서 거짓으로 숨기지 않는다.

---

## Object lifetime

manual storage를 사용하는 container에서는 다음을 지킨다.

- live object만 construct한다.
- 생성되지 않은 raw storage를 object처럼 접근하지 않는다.
- placement new 이후 C++17 object lifetime 규칙을 지킨다.
- 필요하면 `std::launder`를 검토한다.
- live object는 정확히 한 번 destroy한다.
- `pop`, `clear`, destructor에서 lifetime이 중복 종료되지 않게 한다.
- mutation failure 시 기존 object를 불필요하게 잃지 않는다.
- overwrite는 어떤 object의 lifetime을 종료하는지 명확히 한다.
- self-alias / overlapping source가 lifetime 종료 후 dangling reference가 되지 않게 한다.

---

## Container 규칙

`StaticQueue` 같은 fixed-capacity container는 다음을 지킨다.

- capacity는 compile-time 또는 명시적인 fixed contract로 유지한다.
- metadata를 불필요하게 키우지 않는다.
- small-index optimization을 깨는 변경은 측정과 이유 없이 하지 않는다.
- full/empty semantics를 명확히 한다.
- overwrite behavior는 implicit하게 숨기지 않는다.
- move-only / non-trivial type의 lifetime을 고려한다.
- checked API와 std-style API의 균형은 safety를 우선해 결정한다.

`StaticQueue`의 representation을 API rename 때문에 불필요하게 바꾸지 않는다.

---

## Synchronization

generic synchronization은 특정 OS mutex에 종속되지 않는다.

기본 mutex contract:

```cpp
mutex.lock();
mutex.unlock();
```

규칙:

- virtual mutex interface를 만들지 않는다.
- portable/deterministic code에서 `std::mutex`, FreeRTOS mutex, pthread mutex를 직접 사용하지 않는다.
- platform-specific mutex는 adapter 계층으로 분리한다.
- RAII lock을 우선 사용한다.
- synchronization이 필요 없는 경우 `NullMutex` 같은 compile-time 대체 방식을 사용할 수 있다.
- thread-safe wrapper가 내부 raw pointer/reference를 lock 밖으로 노출하지 않게 한다.
- check + mutation이 atomic이어야 하는 operation은 하나의 lock scope에서 수행한다.
- sink/formatter처럼 queue lock이 필요 없는 작업을 mutex 안에서 수행하지 않는다.

---

## Logging architecture

logger의 역할을 다음처럼 분리한다.

```text
Logger     = logging algorithm
Queue      = buffering / storage policy
Mutex      = synchronization policy
Clock      = timestamp source
Formatter  = presentation policy
Sink       = output destination
```

규칙:

- 특정 queue implementation을 logger algorithm에 불필요하게 고정하지 않는다.
- 특정 output destination을 logger에 직접 박지 않는다.
- deterministic queue configuration을 보존하면서 host storage를 opt-in으로 확장할 수 있게 한다.
- file, serial, stdout, UDP 같은 destination-specific 기능은 sink/adapter 계층을 우선한다.
- runtime formatter state와 record storage state를 불필요하게 결합하지 않는다.
- filter timing, timestamp capture timing, formatter timing을 명시적으로 유지한다.

현재 중요한 timing contract:

```text
level filter
→ log / enqueue-time

timestamp capture
→ enqueue-time

runtime color
→ drain / format-time

runtime UTC offset
→ drain / format-time
```

record에 formatter/runtime presentation state를 불필요하게 복제하지 않는다.

---

## Queue backend 확장

향후 AsyncLogger는 서로 다른 queue backend를 사용할 수 있도록 확장할 수 있다.

예:

```text
StaticQueue
→ fixed capacity
→ deterministic
→ zero-heap path

std::queue
→ host convenience
→ dynamic allocation 가능
```

규칙:

- 두 queue API의 차이를 숨기기 위해 과도한 public abstraction을 만들지 않는다.
- 필요한 경우 작은 internal adapter/traits를 사용한다.
- internal adapter를 사용자가 직접 알아야 하는 public API로 만들지 않는다.
- `std::queue` 선택 시 full/capacity/overwrite semantics가 없음을 명확히 다룬다.
- fixed-capacity policy를 unbounded queue에 억지로 적용하지 않는다.

---

## Sink / I/O

output destination은 sink abstraction으로 분리한다.

예:

```text
StdoutSink
SerialSink
FileSink
UdpSink
TeeSink / FanoutSink
```

규칙:

- logger에 `setFileName()` 같은 destination-specific 상태를 직접 추가하지 않는다.
- File/UDP 등 실패 가능한 backend는 error propagation contract를 먼저 정의한다.
- I/O 실패를 silent success로 숨기지 않는다.
- sink composition에 dynamic collection/type erasure가 꼭 필요한지 먼저 검토한다.
- 가능하면 compile-time/fixed composition을 우선 검토한다.

---

## Platform adapter

platform-specific 기능은 별도 adapter에 둔다.

예:

```text
StdMutex
SystemClock
StdoutSink
StdFileSink

ArduinoSerialSink
ArduinoMillisClock

FreeRtosStaticMutex
```

규칙:

- adapter가 필요한 platform header를 include하는 것은 허용한다.
- 그 include가 unrelated generic header로 transitively 퍼지지 않게 한다.
- platform adapter가 generic utility를 사용하는 방향은 허용한다.
- generic utility가 platform adapter를 알아서는 안 된다.

---

## V1 / V2 작업 규칙

- V2 작업 중 명시적인 migration 단계가 아니면 V1 코드를 수정하지 않는다.
- 한 Step에서 관련 없는 refactoring을 섞지 않는다.
- 이전 Step에서 확정한 contract를 임의로 변경하지 않는다.
- breaking change가 필요한 경우 이유와 영향 범위를 먼저 검토한다.
- V2는 API compatibility보다 최종 API clarity를 우선할 수 있다.
- 하지만 이미 확정한 V2 contract를 이유 없이 흔들지 않는다.

---

## V1 기능 보존

- V2는 breaking refactor를 허용하지만 V1의 유용한 기능은 가능한 한 유지한다.
- API와 내부 구현은 변경할 수 있지만 기존 기능을 이유 없이 제거하지 않는다.
- 기존 기능이 현재 구조와 맞지 않으면 삭제보다 적절한 optional/platform/sink/storage layer로 이동하는 방안을 우선 검토한다.
- ANSI color logging 같은 선택 기능도 V2에서 사용할 수 있게 유지한다.
- V1 기능을 대체하거나 제거해야 할 경우 구현 전에 이유와 대체 방법을 명확히 검토한다.
- 최종 V2 완료 전 V1과 V2의 feature parity를 점검한다.

---

## 테스트

production 코드 변경 후 가능한 범위에서 다음을 확인한다.

```text
Debug
Release
MSVC ASan
standalone public header compile
compile-time contracts
기존 regression tests
git diff --check
```

규칙:

- 기존 regression check 수를 이유 없이 줄이지 않는다.
- 신규 compiler warning을 만들지 않는다.
- 신규 C4819를 만들지 않는다.
- 기존 V1 warning은 현재 Step과 직접 관련 없으면 수정하지 않는다.
- test가 implementation detail을 과도하게 ABI contract로 고정하지 않게 한다.
- x64 `sizeof` 결과를 MCU ABI로 일반화하지 않는다.
- representation 변경이 없는지 확인할 목적의 size regression은 허용한다.

ASan은 Windows 일반 PowerShell에서 runtime DLL 경로 문제로 실패할 수 있다.

Visual Studio Developer 환경을 사용한다.

ASan runtime DLL을 프로젝트 폴더나 System32에 복사하지 않는다.

---

## Portability 테스트 관점

generic/deterministic component의 test는 host-only assumption을 만들지 않는다.

예:

- 16-bit `size_t` 가능성을 고려한다.
- host 전용 크기를 portable public ABI contract로 고정하지 않는다.
- 특정 desktop filesystem/clock behavior를 generic test에 섞지 않는다.
- host adapter test는 host dependency를 사용할 수 있다.
- host-only test 때문에 portable public header가 host dependency를 요구하게 만들지 않는다.

---

## 성능 및 크기

다음 표현은 측정 결과가 있을 때만 사용한다.

```text
더 빠르다
zero-cost다
RAM을 절약한다
flash를 줄인다
더 효율적이다
```

최적화 전에는 correctness와 memory safety를 우선한다.

작은 RAM 절감을 위해 다음을 희생하지 않는다.

- object lifetime correctness
- overflow safety
- alias safety
- API clarity
- portability

---

## "가볍다"의 의미

`cmsUtils`에서 "가볍다"는 모든 component가 무조건 heap을 쓰지 않는다는 뜻이 아니다.

다음 의미를 가진다.

### Deterministic component

- runtime heap allocation 없음
- predictable memory usage
- small/fixed metadata
- 불필요한 runtime dependency 없음
- 불필요한 object construction 없음

### Host / optional component

- 기능에 필요한 resource cost를 숨기지 않음
- 사용하지 않는 기능의 dependency/cost를 다른 component에 강요하지 않음
- dynamic allocation을 사용하는 경우 opt-in임이 명확함

공통 원칙:

```text
불필요한 비용을 사용자에게 강요하지 않는다.
```

작은 메모리 절감을 위해 correctness와 safety를 희생하지 않는다.

---

## Git

commit message는 자연스럽고 짧은 한국어로 작성한다.

예:

```text
V2 정적 큐 추가
V2 동기화 기반 추가
V2 UTC 오프셋 시간 표시 지원 추가
V2 큐 full overwrite 정책 추가
프로젝트 개발 규칙 보완
```

사용자가 명시적으로 요청하지 않으면:

- commit하지 않는다.
- push하지 않는다.

단, ChatGPT review를 통해 특정 Step commit 승인이 이미 완료된 경우에는 그 승인 범위만 commit할 수 있다.

review용 산출물은 commit하지 않는다.

예:

```text
CODEX_CMS_EMBEDDED_UTILS_REVIEW.md
V2_STEP*_REVIEW.diff
V2_STEP*_FINAL_REVIEW.diff
```

향후 파일명이 바뀌더라도 review 산출물이라는 성격이 같으면 commit하지 않는다.

unrelated untracked 파일을 임의로 삭제하거나 수정하지 않는다.

`AGENTS.md` 변경은 기능 구현 commit에 실수로 섞지 않는다.

개발 규칙 변경은 가능하면 별도 commit한다.

---

## 작업 방식

작업 시작 시 먼저 확인한다.

```bash
git status
git branch --show-current
git log -8 --oneline
git remote -v
```

현재 worktree에 기존 사용자 변경이 있으면 보존한다.

작업 전:

- 현재 Step 기준 HEAD 확인
- 기존 review diff 존재 여부 확인
- unrelated modification 확인
- 현재 repository/remote 이름 확인

작업 완료 후 확인:

```bash
git diff --check
git status
git diff --stat
```

큰 변경은 바로 commit하지 않고 review 가능한 상태에서 멈춘다.

기본 workflow:

```text
구현
→ build/test
→ review diff 생성
→ stage/commit/push 금지
→ ChatGPT review
→ 승인 후 commit
```

---

## Repository / project naming

현재 프로젝트 이름은 `cmsUtils`다.

기존 이름:

```text
cms-embedded-utils
```

은 migration 과정에서 제거한다.

C++ namespace는 repository 이름과 별개로:

```text
cms::util
```

을 사용한다.

CMake project/target/export alias의 정확한 spelling은 build-system migration 단계에서 일관되게 확정한다.

이름 변경 과정에서 old/new project identifier가 불필요하게 장기간 공존하지 않게 한다.

---

## 현재 V2 logging 중요 contract

다음은 이미 확정된 contract이므로 관련 Step이 아니면 변경하지 않는다.

### Level

```text
trace
debug
info
warning
error
critical
```

### Default full queue behavior

```text
RejectOnFull
→ Status::no_space
→ FIFO unchanged
```

### V1-compatible overwrite opt-in

```text
OverwriteOldestOnFull
→ oldest 제거
→ newest 저장
→ Status::ok
```

### Runtime level filter

```text
log/enqueue-time
```

filtered/disabled log:

```text
Status::ok
Clock 호출 없음
queue 접근 없음
message size 검사 없음
```

### Runtime ANSI color

```text
drain/format-time
```

### Runtime UTC offset

```text
drain/format-time
```

default:

```text
UTC offset = 0
```

지원 범위:

```text
-720 ~ +840 minutes
```

automatic timezone detection, IANA timezone, DST calculation은 library가 수행하지 않는다.

---

## 현재 Step 이후 개발 방향

현재 Step 17 이후 예상 순서:

```text
Step 18
cmsUtils public API / namespace / container API alignment

Step 19
AsyncLogger queue storage abstraction
- StaticQueue
- std::queue

Step 20
Sink contract
- FileSink
- Tee/Fanout

Step 21
global/singleton/dispatch routing 검토

Step 22
UDP Sink

Final
V1/V2 feature parity
migration documentation
full validation
2.0.0 preparation
```

각 Step의 정확한 범위는 이전 Step review 후 확정한다.

---

## 기본 원칙

이 프로젝트의 목표는 "작은 코드" 자체가 아니다.

좋은 `cmsUtils` component는 다음 특성을 가진다.

- 역할과 resource contract가 명확하다.
- 사용하지 않는 기능의 비용을 강요하지 않는다.
- deterministic path는 실제로 deterministic하다.
- host convenience는 명시적인 opt-in이다.
- dependency 방향이 단순하다.
- lifetime과 ownership이 분명하다.
- 실패 semantics가 예측 가능하다.
- 표준 C++ 사용자에게 가능한 범위에서 익숙하다.
- platform 차이를 core에 숨은 형태로 섞지 않는다.

Correctness와 Safety가 항상 작은 메모리 절감보다 우선한다.
