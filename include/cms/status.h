#pragma once

#include <cstddef>
#include <cstdint>

namespace cms {

enum class Status : std::uint8_t {
    ok,
    no_space,
    invalid_argument,
    invalid_utf8,
    out_of_range,
    unsupported
};

struct WriteResult {
    // written is the payload byte count actually produced by this operation.
    // required is the complete payload byte count without truncation. Neither
    // count includes pre-existing destination bytes or a terminating NUL byte.
    // Transactional success has written == required. Transactional failure has
    // written == 0. Explicit truncation reports no_space and the partial count.
    Status status;
    std::size_t written;
    std::size_t required;
};

template<class T>
struct ParseResult {
    Status status;
    // T must be value-initializable.
    T value{};
    std::size_t consumed;
};

} // namespace cms
