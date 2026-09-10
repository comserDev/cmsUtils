# INI 문서

`cms::util::ini::Document`는 INI 파일의 내용을 메모리에 보관하고 `section`/`key`
단위로 값을 조회하거나 수정하는 문서 모델이다. Host와 ESP32에서 공통으로 사용할 수
있으며, 주석·빈 줄·알 수 없는 줄과 기존 줄 순서를 가능한 한 그대로 유지한다.
중복 `key`, global/root section, LF/CRLF와 final newline 보존, load 실패 시 기존
문서를 유지하는 규칙도 함께 제공한다.

`Document`는 ESP32에서 사용할 수 있지만 deterministic zero-heap component는
아니다. 다음과 같은 동적 메모리 자원을 사용할 수 있는 편의 기능이다.

```text
std::string
std::vector
std::unordered_map
```

## Host 사용

Host에서는 `<cms/util/ini/file.h>`의 `loadFile`과 `saveFile`을 사용한다.

```cpp
#include <cms/util/ini/document.h>
#include <cms/util/ini/file.h>

cms::util::ini::Document config;

cms::util::ini::loadFile(
    "config.ini",
    config);

config["Network"]["port"] = "8080";

auto port =
    config["Network"]["port"].get();

cms::util::ini::saveFile(
    "config.ini",
    config);
```

## ESP32 사용

ESP32에서는 `<cms/util/ini/arduino_file.h>`의 overload를 사용한다.

```cpp
#include <LittleFS.h>
#include <cms/util/ini/document.h>
#include <cms/util/ini/arduino_file.h>

cms::util::ini::Document config;

cms::util::ini::loadFile(
    LittleFS,
    "/config.ini",
    config);

config["WiFi"]["ssid"] = "NAMU001";

auto ssid =
    config["WiFi"]["ssid"].get();

cms::util::ini::saveFile(
    LittleFS,
    "/config.ini",
    config);
```

Arduino overload는 `fs::FS`를 기반으로 하므로 LittleFS, SPIFFS, SD 등
`fs::FS` 호환 파일시스템에서 같은 `loadFile`/`saveFile` API를 사용할 수
있다. `document.h`와 `file.h`는 Arduino SDK 없이 Host에서 include할 수 있고,
Arduino 파일시스템 header는 `arduino_file.h`에서만 사용한다.

## TextMode와 문자 인코딩

`loadFile`은 기본적으로 `TextMode::automatic`을 사용하므로 일반적인 경우 mode를
지정할 필요가 없다. Host와 ESP32 overload 모두 다음 세 가지 mode를 지원한다.

```cpp
enum class TextMode {
    automatic,
    ascii,
    utf8
};
```

`automatic`에서는 파일 시작의 UTF-8 BOM(`EF BB BF`)을 먼저 확인한다. BOM이 있으면
BOM을 문법 데이터에서 제거하고 UTF-8로 처리한다. BOM이 없으면 전체 입력을 검사해
올바른 UTF-8이면 UTF-8 mode를 사용하고, 검사에 실패하면 byte/ASCII mode로 처리한다.
ASCII-only 입력은 올바른 UTF-8이므로 UTF-8 mode로 판정되며, 이는 정상적인 동작이다.

```cpp
cms::util::ini::loadFile(
    "config.ini",
    config,
    cms::util::ini::TextMode::utf8);
```

`TextMode::utf8`은 입력 전체가 올바른 UTF-8인지 확인한다. 잘못된 입력이면
`Status::invalid_utf8`를 반환하고 기존 `Document`를 그대로 유지한다. UTF-8 mode와
automatic mode에서 파일 시작의 BOM은 허용되며 문법 데이터에는 포함되지 않는다. 파일
중간의 BOM byte는 일반 데이터로 보존한다.

`TextMode::ascii`는 UTF-8 검사를 요구하지 않고 section/key/value를 byte sequence로
처리한다. 양끝의 공백은 `cms::util::string::trim()`이 제거하는 ASCII whitespace만
대상이며, non-ASCII whitespace는 값의 일부로 남는다.

UTF-8 mode에서는 `cms::util::utf8::trim()`이 Unicode White_Space code point를
기준으로 양끝을 제거한다. 지원 범위는 U+0009~U+000D, U+0020, U+0085, U+00A0,
U+1680, U+2000~U+200A, U+2028, U+2029, U+202F, U+205F, U+3000이다.

따라서 다음과 같은 UTF-8 section/key/value를 그대로 사용할 수 있다.

```ini
[네트워크]
이름 = 나무001
설명 = 자전거 경기 계측 장치
```

U+3000 전각 공백이 문법 양끝에 있어도 automatic/UTF-8 mode에서는 공백으로
처리한다. ASCII mode에서는 U+3000을 제거하지 않는다.

## 명시적 API와 proxy API

명시적 API는 다음과 같다.

```cpp
void clear();
std::string getValue(
    const std::string& section,
    const std::string& key,
    const std::string& defaultValue = {}) const;
Status setValue(
    const std::string& section,
    const std::string& key,
    const std::string& value);
bool contains(const std::string& section, const std::string& key) const;
```

`operator[]`는 `SectionProxy`와 `ValueProxy`를 통해 같은 문서를 편하게
조회하고 수정하는 API다. 대입은 별도 규칙을 사용하지 않고 `setValue()`를
호출하므로 중복 `key`, global section, 줄 순서 보존 규칙이 명시적 API와 같다.

```cpp
config["Network"]["ssid"] = "MyWifi";

std::string ssid =
    config["Network"]["ssid"];

auto port =
    config["Network"]["port"].get();

bool present =
    config["Network"].contains("ssid");
```

다음 코드는 문자열 값을 얻는다.

```cpp
std::string value =
    config["Section"]["Key"];
```

반면 다음 코드는 C++ `auto` 타입 추론에 따라 `ValueProxy` 타입을 얻는다.

```cpp
auto value =
    config["Section"]["Key"];
```

`auto`로 실제 문자열을 받으려면 `.get()`을 사용한다.

```cpp
auto value =
    config["Section"]["Key"].get();
```

존재하지 않는 값을 읽는 것은 문서를 수정하지 않는다.

```cpp
auto value =
    config["Missing"]["Key"].get();
```

이 코드는 빈 문자열을 반환할 뿐 `section`이나 `key`를 만들지 않는다. 실제
수정은 다음 대입에서만 발생한다.

```cpp
config["Missing"]["Key"] = "value";
```

`ValueProxy::exists()`는 key 존재 여부를, `SectionProxy::contains()`는 해당
section 안의 key 존재 여부를 반환한다. proxy는 `Document`와 section/key 이름만
보관하므로 다른 key 삽입으로 내부 vector가 재할당되어도 안전하다. `const
Document`에서는 읽기 전용 proxy만 제공한다.

빈 section 이름 `""`은 global/root section을 뜻한다. global key를 추가해도
`[]` header는 만들지 않는다.

```cpp
config[""]["version"] = "2";
```

## 파싱과 보존 규칙

- ASCII whitespace를 제거한 뒤 빈 줄이면 빈 줄로 보존한다.
- 첫 non-whitespace character가 `;` 또는 `#`이면 주석 줄이다.
- `[Section]` 형식의 named section을 인식하며 section 이름 양끝을 trim한다.
- `[]`와 `[   ]`는 유효한 section이 아니다.
- 첫 번째 `=`에서만 key와 value를 나눈다.
- key와 value 양끝의 ASCII whitespace를 trim한다.
- inline comment 문법은 지원하지 않으므로 `value ; comment` 전체가 value다.
- section과 key는 case-sensitive다.

주석, 빈 줄, 알 수 없는 줄, `section`/`key` 순서와 수정하지 않은 줄의 원문을
보존한다. 수정한 `key`/`value` 줄은 `key = value` 형식으로 기록할 수 있다.

같은 `section`/`key`의 중복을 허용하며 조회와 수정에는 마지막 항목을 사용한다.
새 `key`는 마지막 같은 section block에 추가하고, 새 global `key`는 첫 named
section 앞에 삽입한다.

직렬화할 때 입력의 LF 또는 CRLF를 유지하고 final newline 존재 여부도 유지한다.
혼합 줄바꿈 입력은 처음 발견한 newline style을 사용한다.

Host와 Arduino의 `loadFile`은 파일을 읽고 임시 파싱 상태를 구성한 뒤 성공한
경우에만 `Document`에 반영한다. 파일 open/read/parse가 실패하면 기존 문서
내용은 그대로 남는다.
