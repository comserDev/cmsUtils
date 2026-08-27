# Binary utilities API contract

이 문서의 API는 `cms::util`의 deterministic / zero-heap component다. C++17에서
exception과 RTTI 없이 사용할 수 있으며 OS, transport, protocol schema에 의존하지 않는다.

## Byte storage

- `ByteView(data, size)`는 binary data를 소유하지 않는다. `nullptr` 입력은 빈 view로
  canonicalize하며 embedded NUL을 보존한다. `subview()`는 끝에서 clamp한다.
- `ByteBuffer(data, capacity, size)`는 caller-owned storage와 size state를 alias한다.
  `size <= capacity`여야 하며 zero-capacity buffer는 `data == nullptr`을 허용한다.
  `commit()` 실패는 기존 size를 유지한다.
- `StaticByteBuffer<N>`은 `N > 0`인 storage를 직접 소유하며 heap을 사용하지 않는다.
  `buffer()`로 얻은 alias는 원본보다 오래 사용할 수 없다.

## BinaryReader

`BinaryReader(ByteView)`는 `readUint8`, `readUint16BigEndian`,
`readUint32BigEndian`, `readUint64BigEndian`, `readBytes`, `skip`을 제공한다.
Multi-byte integer는 byte 단위 shift로 읽으므로 alignment와 host endian에 의존하지 않는다.
입력이 부족하면 `Status::out_of_range`이며 cursor와 output argument를 변경하지 않는다.

## BinaryWriter

`BinaryWriter(ByteBuffer)`는 대응하는 unsigned big-endian write와 `writeBytes`를
제공한다. 기존 buffer 끝에서 시작하고 성공할 때마다 shared size가 cursor로 전진한다.
공간 부족은 `Status::no_space`, invalid buffer는 `Status::invalid_argument`이며 실패 시
buffer content, size, writer position을 변경하지 않는다. `writeBytes`는 겹치는 source와
destination을 지원한다.

## CRC-32/ISO-HDLC

`crc32::isoHdlc(ByteView)`는 one-shot 계산을 제공한다. `crc32::IsoHdlc`의
`update()`와 `value()`는 chunk 단위 계산을 제공하고 `reset()`은 초기 상태로 돌아간다.
parameter는 poly `0x04C11DB7`, reflected poly `0xEDB88320`, init/xorout
`0xFFFFFFFF`, refin/refout `true`다. ASCII `123456789`의 결과는 `0xCBF43926`이다.

TLV, frame header, message ID, session, ACK, retry 같은 protocol 의미는 이 API 범위가 아니다.
