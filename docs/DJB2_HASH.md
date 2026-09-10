# DJB2 해시

cmsUtils V2는 `<cms/util/hash/djb2.h>`에서 범용 DJB2 utility를 제공한다. 입력
byte를 그대로 처리하는 비암호학적 해시이며, 모든 연산을 32-bit 범위에서
wraparound한다.

```text
hash = 5381
hash = hash * 33 + byte
```

public API에는 상태를 보관하는 `cms::util::hash::Djb2`와 `ByteView`,
`StringView`를 받는 one-shot `cms::util::hash::djb2()` overload가 있다.
`StringView`의 각 byte는 unsigned byte로 처리하며, 해시 내부에서 대소문자를
변환하지 않는다. embedded NUL을 포함한 모든 binary byte를 입력으로 사용할 수
있다.

styled ANSI formatter는 기존 V1 색상 매핑을 유지하기 위해 tag byte를 ASCII
uppercase로 변환하는 표현 정책을 적용한 뒤 `Djb2`에 전달한다.

DJB2는 cryptographic hash가 아니므로 authentication, integrity protection,
persistent globally unique identifier, collision resistance에 사용하면 안 된다.
프로토콜에서 요구하는 목적에 따라 SHA-256이나 CRC-32를 선택하고, 인증에는 별도
HMAC 구현을 사용한다.

public 선언은 [V2 API 레퍼런스](API_REFERENCE.md)에서 확인할 수 있다.
