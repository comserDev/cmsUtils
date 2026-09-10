# SHA-256, HMAC-SHA256과 digest 비교

cmsUtils는 Host와 ESP32에서 공통으로 사용할 수 있는 SHA-256, HMAC-SHA256과
constant-time 성격의 byte 비교 API를 제공한다. 모두 C++17 기반이며 동적 메모리,
filesystem, Arduino runtime에 의존하지 않는다.

## SHA-256

`sha256()`는 입력 byte 전체의 SHA-256 digest를 계산한다.

```cpp
#include <cms/util/crypto/sha256.h>

std::uint8_t digest[cms::util::crypto::Sha256DigestSize] = {};
const cms::util::Status status = cms::util::crypto::sha256(
    cms::util::ByteView(data, dataSize),
    digest);
```

SHA-256은 key를 사용하지 않는 hash 함수다. 데이터의 digest가 같다는 사실만으로
상대방의 신원이나 메시지의 출처를 인증할 수 없다. Secret이 필요한 인증 용도에서
`sha256(secret || message)` 같은 독자적인 구성을 만들지 않는다.

## HMAC-SHA256

`hmacSha256()`는 RFC 2104/RFC 4231 방식으로 key와 data를 결합한다.

```cpp
#include <cms/util/crypto/hmac_sha256.h>

std::uint8_t digest[cms::util::crypto::Sha256DigestSize] = {};
const cms::util::Status status = cms::util::crypto::hmacSha256(
    cms::util::ByteView(key, keySize),
    cms::util::ByteView(data, dataSize),
    digest);
```

Empty key/data와 embedded NUL을 포함한 임의 binary input을 처리한다. Key가 SHA-256
block 크기인 64 byte를 넘으면 먼저 SHA-256으로 줄인 뒤 HMAC 계산에 사용한다.
API가 반환하는 값은 문자열이 아닌 raw 32-byte digest다. Hex나 Base64 변환은
crypto core의 책임이 아니다.

## HMAC 결과 비교

HMAC key의 생성, 저장, 교체와 폐기는 caller가 안전하게 관리해야 한다. cmsUtils는
key storage나 Namu001 전용 secret 및 인증 transcript를 제공하지 않는다.

계산한 HMAC과 수신한 MAC을 비교할 때는 일반 `memcmp()` 대신
`constantTimeEqual()`을 사용한다.

```cpp
#include <cms/util/crypto/constant_time.h>

const bool authenticated = cms::util::crypto::constantTimeEqual(
    cms::util::ByteView(calculated),
    receivedMac);
```

길이가 다르면 즉시 `false`를 반환한다. 길이가 같으면 첫 mismatch 위치에서
종료하지 않고 모든 byte를 확인한다. 데이터 값에 따른 명시적인 조기 종료를 피하는
portable C++ 구현이지만, 모든 compiler와 CPU에서 wall-clock 실행 시간이 절대적으로
같다고 보증하는 API는 아니다.

로그에 key나 digest를 자동으로 출력하지 않는다.

## 결과와 자원 사용 규칙

SHA-256과 HMAC-SHA256은 caller가 제공한 32-byte 배열에 결과를 기록한다. 입력 검증에 실패하면
기존 digest를 유지한다. 내부에서는 고정 크기 stack buffer와 private SHA-256
streaming state만 사용하며 heap allocation, exception, RTTI가 필요하지 않다.

`constantTimeEqual()`도 heap을 사용하지 않고 입력을 변경하지 않는다.

Public streaming SHA-256/HMAC API, HMAC verify wrapper, 암호화, random 생성,
key derivation은 제공하지 않는다.
