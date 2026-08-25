# cms-embedded-utils 개발 규칙

## 프로젝트 방향

이 라이브러리는 embedded 환경을 우선한다.

설계 우선순위:

1. Correctness
2. Memory Safety
3. Zero Heap / Deterministic Memory
4. Small RAM Footprint
5. API Clarity
6. Portability
7. Code Size
8. Testability
9. Performance

성능이나 메모리 절감 효과는 실제 측정 없이 주장하지 않는다.

---

## 메모리 정책

- core에서는 dynamic allocation을 사용하지 않는다.
- `new/delete`, `malloc/free`를 사용하지 않는다.
- `std::string`, `std::vector`, `std::deque`, `std::queue`, `std::function`처럼 heap allocation 가능성이 있는 타입을 core에 넣지 않는다.
- placement new는 object lifetime 관리 목적으로만 허용한다.
- fixed-capacity / compile-time storage를 우선한다.
- heap fragmentation이 발생하지 않는 구조를 유지한다.
- 장시간 동작하는 MCU에서도 메모리 사용량이 예측 가능해야 한다.
- 불필요한 object construction과 initialization을 피한다.

---

## Embedded portability

- core는 OS, RTOS, 특정 MCU framework에 종속되지 않는다.
- core에서 `<mutex>` 및 `std::mutex`에 의존하지 않는다.
- FreeRTOS, Arduino, Windows, POSIX 등 platform-specific 기능은 adapter 계층으로 분리한다.
- exception과 RTTI에 의존하지 않는다.
- bare-metal 또는 축소된 C++ runtime에서도 사용할 수 있는 구조를 우선한다.
- core에 불필요한 standard library runtime dependency를 추가하지 않는다.

---

## C++ 기준

- core의 최소 language standard는 C++17로 유지한다.
- C++17을 선택한 이유는 embedded toolchain 호환성과 portability다.
- 최신 compiler를 사용하는 것과 최소 language standard를 높이는 것은 구분한다.
- C++20/23 기능은 단순한 문법 편의를 위해 core의 최소 요구사항을 높이는 용도로 사용하지 않는다.
- 상위 C++ 표준으로 전환할 때는 실제 지원 대상 MCU/toolchain 호환성을 먼저 검증한다.
- 기본 namespace는 `cms`를 사용한다.
- 하위 namespace는 lower-case로 작성한다.
- public header는 `include/cms/` 아래에 둔다.
- header 확장자는 `.h`를 사용한다.
- 파일명은 `lower_snake_case`를 사용한다.

예:

```text
include/cms/static_queue.h
include/cms/string_buffer.h
include/cms/sync/lock_guard.h
```

## Naming

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

다음과 같은 leading underscore 스타일은 사용하지 않는다.

```cpp
_size
_head
_data
```

---

## 주석

- 모든 신규 또는 수정 주석은 UTF-8로 작성한다.
- 주석은 자연스러운 한국어를 기본으로 한다.
- 존댓말을 사용하지 않는다.
- 개발자가 코드에 직접 작성한 것 같은 짧고 자연스러운 문체를 사용한다.
- 전문용어, C++ 용어, API 이름은 필요한 경우 영어를 그대로 사용한다.
- 억지로 모든 기술 용어를 한국어로 번역하지 않는다.
- 코드만 읽어도 명백한 내용을 그대로 반복 설명하지 않는다.
- 지나치게 길거나 장황한 주석은 피한다.

주석은 주로 다음 내용을 설명할 때 사용한다.

- 왜 이런 구현 방식을 사용했는가
- 중요한 contract가 무엇인가
- lifetime 또는 memory safety에서 주의할 점이 무엇인가
- overflow, aliasing, object lifetime 등 코드만으로 의도를 파악하기 어려운 부분

좋은 예:

```cpp
// full 상태에서는 constructor를 호출하지 않는다.

// head + size를 먼저 계산하지 않아 size_t overflow를 피한다.

// INT64_MIN은 직접 negate하지 않고 unsigned magnitude로 변환한다.

// lock을 유지하는 동안에만 front reference가 유효하다.
```

피해야 할 예:

```cpp
// 이 함수는 현재 큐가 가득 차 있는지 확인한 다음,
// 가득 차 있는 경우에는 새로운 객체를 생성하지 않고...
```

주석 때문에 신규 encoding warning이 발생하지 않도록 파일은 UTF-8을 유지한다.

기존 V1 파일의 encoding 문제를 해결한다는 이유로 파일 전체를 일괄 변환하지 않는다.

---

## Zero Heap 원칙

core production 코드에서는 다음을 사용하지 않는다.

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

Zero Heap은 단순한 스타일 규칙이 아니라 다음 목적을 가진다.

- heap fragmentation 방지
- 장시간 동작 안정성
- deterministic memory usage
- MCU RAM 사용량 예측 가능성

---

## Template 사용

- compile-time polymorphism이 적합한 경우 template을 사용한다.
- runtime polymorphism이 필요하지 않은 곳에서 virtual interface를 만들지 않는다.
- vtable과 type erasure를 불필요하게 추가하지 않는다.
- `std::function` 대신 callback template을 우선한다.
- template 사용으로 flash/code size가 과도하게 증가할 가능성도 고려한다.
- 측정 없이 template이 항상 zero-cost라고 주장하지 않는다.

---

## Public API

- public API는 작고 명확하게 유지한다.
- 새로운 public type이나 함수는 실제 필요성이 있을 때만 추가한다.
- public API를 추가하기 전에 lifetime, ownership, error semantics를 먼저 정의한다.
- raw pointer/reference를 반환할 경우 lifetime contract를 명확히 한다.
- synchronization 보호가 끝난 뒤 위험해지는 pointer/reference를 public API로 노출하지 않는다.
- 새로운 enum이나 result type을 불필요하게 만들지 않고 기존 `cms::Status`를 우선 재사용한다.

---

## Error 처리

- core에서 exception을 error handling 방식으로 사용하지 않는다.
- 가능한 경우 `cms::Status`, `WriteResult`, `ParseResult<T>` 같은 명시적 result type을 사용한다.
- 실패 시 destination을 변경하지 않는 transactional semantics를 우선한다.
- truncation은 명시적으로 요청된 API에서만 허용한다.
- overflow가 가능한 연산은 계산 전에 검증한다.

---

## Object lifetime

manual storage를 사용하는 container에서는 다음을 지킨다.

- live object만 construct한다.
- 아직 생성되지 않은 raw storage를 object처럼 접근하지 않는다.
- placement new 이후 C++17 object lifetime 규칙을 지킨다.
- 필요하면 `std::launder`를 사용한다.
- live object는 정확히 한 번 destroy한다.
- `pop`, `clear`, destructor에서 lifetime이 중복 종료되지 않게 한다.
- full operation 실패 시 불필요한 constructor side effect가 발생하지 않게 한다.

---

## Synchronization

- core synchronization은 특정 OS mutex에 종속되지 않는다.
- mutex type은 기본적으로 다음 duck-typing contract를 사용한다.

```cpp
mutex.lock();
mutex.unlock();
```

- virtual mutex interface를 만들지 않는다.
- core에서 `std::mutex`, FreeRTOS mutex, pthread mutex를 직접 사용하지 않는다.
- platform-specific mutex는 adapter 계층으로 분리한다.
- RAII lock을 우선 사용한다.
- synchronization이 필요 없는 경우 `NullMutex` 같은 compile-time 대체 방식을 사용할 수 있다.
- thread-safe wrapper가 내부 raw pointer를 lock 밖으로 노출하지 않게 한다.

---

## V1 / V2 작업 규칙

- V2 작업 중에는 명시적인 migration 단계가 아니면 V1 코드를 수정하지 않는다.
- 한 Step에서 관련 없는 refactoring을 하지 않는다.
- 기존 public API를 필요 없이 변경하지 않는다.
- 이전 Step에서 확정한 contract를 임의로 변경하지 않는다.
- breaking change가 필요한 경우 먼저 이유와 영향 범위를 보고한다.

---

## 테스트

production 코드 변경 후 가능한 범위에서 다음을 확인한다.

- Debug
- Release
- ASan
- standalone public header compile
- 기존 regression tests
- `git diff --check`

신규 compiler warning을 만들지 않는다.

기존 V1 warning은 현재 Step과 직접 관련이 없으면 수정하지 않는다.

ASan은 Windows 일반 PowerShell에서 runtime DLL 경로 문제로 실패할 수 있으므로 Visual Studio Developer 환경에서 실행한다.

ASan runtime DLL을 프로젝트 폴더나 System32에 임의로 복사하지 않는다.

---

## Embedded 테스트 관점

테스트 코드는 host PC에서 실행하더라도 embedded portability를 해치면 안 된다.

예:

- 16-bit `size_t` 가능성을 고려한다.
- host 전용 크기를 public ABI contract로 고정하지 않는다.
- x64 `sizeof` 측정 결과를 MCU ABI로 일반화하지 않는다.
- 테스트 코드가 특정 desktop 환경만 가정해 embedded compiler를 막지 않게 한다.

---

## 성능 및 크기

다음 표현은 측정 결과가 있을 때만 사용한다.

- 더 빠르다
- zero-cost다
- RAM을 절약한다
- flash를 줄인다
- 더 효율적이다

최적화 전에는 correctness와 memory safety를 우선한다.

작은 RAM 절감을 위해 다음을 희생하지 않는다.

- object lifetime correctness
- overflow safety
- API clarity
- portability

---

## Git

- commit message는 자연스러운 한국어로 작성한다.
- 존댓말은 사용하지 않는다.
- 짧고 변경 목적이 바로 보이게 작성한다.

예:

```text
V2 정적 큐 추가
V2 동기화 기반 추가
V2 정수 포맷 및 파싱 추가
V2 문자열 연산 추가
V2 주석을 한국어로 정리
```

사용자가 명시적으로 요청하지 않으면:

- commit하지 않는다.
- push하지 않는다.

다음 파일은 review용 산출물이므로 commit하지 않는다.

```text
CODEX_CMS_EMBEDDED_UTILS_REVIEW.md
V2_STEP*_REVIEW.diff
V2_STEP*_FINAL_REVIEW.diff
```

unrelated untracked 파일을 임의로 삭제하거나 수정하지 않는다.

---

## 작업 방식

작업 시작 시 가능하면 다음을 먼저 확인한다.

```bash
git status
git branch --show-current
git log -8 --oneline
```

작업 완료 후 다음을 확인한다.

```bash
git diff --check
git status
git diff --stat
```

큰 변경은 바로 commit하지 말고 review할 수 있는 상태에서 멈춘다.

사용자가 명시적으로 commit을 요청하기 전에는 commit하지 않는다.

---

## 기본 원칙

이 프로젝트에서 "가볍다"는 것은 단순히 코드 줄 수가 적다는 뜻이 아니다.

다음을 의미한다.

- runtime heap allocation 없음
- predictable RAM usage
- 작은 metadata
- 불필요한 runtime dependency 없음
- 불필요한 object construction 없음
- platform abstraction 비용 최소화
- 장시간 MCU 동작에서 안정적인 memory behavior

단, 작은 메모리 절감을 위해 correctness와 safety를 희생하지 않는다.