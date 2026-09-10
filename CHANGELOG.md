# 변경 기록

## 2.0.0 - 개발 중

- `cms::util` 아래의 C++17 public API와 deterministic/fixed-capacity component를 정리했다.
- 문자열, UTF-8, 숫자 format/parse, byte buffer, binary reader/writer, CRC-32를 제공한다.
- SHA-256과 public DJB2 utility를 추가했다.
- fixed-capacity queue, synchronization wrapper, 조합형 logging과 platform adapter를 제공한다.
- Host와 ESP32에서 공통으로 사용할 수 있는 `cms::util::ini::Document`와 파일 adapter를 추가했다.
- V1 API와의 source compatibility를 유지하지 않고 V2 namespace/header 구조를 확정했다.
