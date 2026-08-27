# DJB2 태그 색상 해시의 위치

이 문서는 V1 logger의 내부 구현을 설명하던 문서다. V1의
`cms::LoggerBase::applyStyling()`은 `[TAG]`에 적용할 ANSI 색상을 고르기 위해 대소문자를
ASCII uppercase로 정규화한 뒤 DJB2 hash를 사용했다.

V2에는 public DJB2 hash API가 없다. 태그 색상 선택은
`cms::util::log::StyledAnsiFormatter`의 presentation detail이며 application이 그 hash 값이나
palette index에 의존하면 안 된다. V1과 같은 표시가 필요한 migration을 위해 현재 formatter는
동등한 tag-color 동작을 유지하지만, 이는 general-purpose hash contract가 아니다.

현재 사용 방법과 안정적인 public contract는 다음 문서를 따른다.

- [V2 API reference](API_REFERENCE.md)
- [V2 examples](EXAMPLES.md)
- [V1에서 V2로 마이그레이션](MIGRATION_V1_TO_V2.md)

DJB2 자체를 protocol integrity, persistent identifier, authentication 또는 collision resistance가
필요한 용도로 사용하면 안 된다. Binary frame의 전송 오류 검출에는 별도로 제공되는
CRC-32/ISO-HDLC API를 사용한다.
