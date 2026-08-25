#pragma once

#include <cstddef>
#include <cstdint>

namespace cms {

// Core API가 공통으로 사용하는 결과 상태다. 세부적인 value와 byte count 계약은
// 각 result 타입과 연산의 public contract를 따른다.
enum class Status : std::uint8_t {
    ok,
    no_space,
    invalid_argument,
    invalid_utf8,
    out_of_range,
    unsupported
};

struct WriteResult {
    // written은 이번 연산이 실제로 기록한 payload byte 수다. required는
    // truncation이 없을 때 필요한 전체 payload byte 수다. 두 값 모두 기존
    // destination의 크기와 terminating NUL은 포함하지 않는다.
    // transactional 연산은 성공하면 written == required이고, 실패하면
    // written == 0이다. 명시적 truncation은 no_space와 실제 기록량을 돌려준다.
    Status status;
    std::size_t written;
    std::size_t required;
};

template<class T>
struct ParseResult {
    Status status;
    // T는 value-initialization이 가능한 타입이어야 한다.
    T value{};
    // consumed의 세부 의미는 각 parser가 정하며 항상 입력 시작 기준 byte 수다.
    std::size_t consumed;
};

} // namespace cms
