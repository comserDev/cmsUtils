#include <cms/util/utf8.h>

#include <limits>

#include <cms/util/detail/utf8_decoder.h>

namespace cms {
namespace util {
namespace utf8 {

namespace {

constexpr char replacement[] = {
    static_cast<char>(0xEF),
    static_cast<char>(0xBF),
    static_cast<char>(0xBD)
};

bool additionOverflows(std::size_t value, std::size_t increment) noexcept {
    return increment > (std::numeric_limits<std::size_t>::max)() - value;
}

bool isUnicodeWhitespace(char32_t value) noexcept {
    return (value >= 0x0009 && value <= 0x000D)
        || value == 0x0020
        || value == 0x0085
        || value == 0x00A0
        || value == 0x1680
        || (value >= 0x2000 && value <= 0x200A)
        || value == 0x2028
        || value == 0x2029
        || value == 0x202F
        || value == 0x205F
        || value == 0x3000;
}

WriteResult failure(Status status) noexcept {
    return {status, 0, 0};
}

} // namespace

DecodeResult decodeNext(StringView input, std::size_t offset) noexcept {
    return detail::utf8::decodeNext(input, offset);
}

Status validate(StringView input) noexcept {
    std::size_t offset = 0;
    while (offset < input.size()) {
        const DecodeResult decoded = detail::utf8::decodeNext(input, offset);
        if (decoded.status != Status::ok) {
            return Status::invalid_utf8;
        }

        offset += decoded.bytes;
    }

    return Status::ok;
}

StringView trim(StringView value) noexcept {
    std::size_t begin = 0;
    while (begin < value.size()) {
        const DecodeResult decoded =
            detail::utf8::decodeNext(value, begin);
        if (decoded.status != Status::ok
            || !isUnicodeWhitespace(decoded.codePoint)) {
            break;
        }
        begin += decoded.bytes;
    }

    std::size_t end = begin;
    std::size_t offset = begin;
    while (offset < value.size()) {
        const DecodeResult decoded =
            detail::utf8::decodeNext(value, offset);
        if (decoded.status != Status::ok) {
            // 잘못된 byte는 공백이 아닌 데이터다. 이 view에는 오류를 반환할
            // 방법이 없으므로 이후 byte까지 그대로 보존한다.
            return value.substr(begin, value.size() - begin);
        }

        offset += decoded.bytes;
        if (!isUnicodeWhitespace(decoded.codePoint)) {
            end = offset;
        }
    }

    return value.substr(begin, end - begin);
}

ParseResult<std::size_t> count(StringView input) noexcept {
    std::size_t offset = 0;
    std::size_t codePoints = 0;

    while (offset < input.size()) {
        const DecodeResult decoded = detail::utf8::decodeNext(input, offset);
        if (decoded.status != Status::ok) {
            return {Status::invalid_utf8, codePoints, offset};
        }

        offset += decoded.bytes;
        ++codePoints;
    }

    return {Status::ok, codePoints, input.size()};
}

WriteResult substring(
    StringView input,
    std::size_t firstCodePoint,
    std::size_t requestedCount,
    StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }

    std::size_t offset = 0;
    std::size_t codePointIndex = 0;
    std::size_t selectedCount = 0;
    std::size_t startByte = 0;
    std::size_t endByte = 0;
    bool startFound = firstCodePoint == 0;

    while (offset < input.size()) {
        const DecodeResult decoded = detail::utf8::decodeNext(input, offset);
        if (decoded.status != Status::ok) {
            return failure(Status::invalid_utf8);
        }

        if (!startFound && codePointIndex == firstCodePoint) {
            startByte = offset;
            endByte = offset;
            startFound = true;
        }

        const std::size_t nextOffset = offset + decoded.bytes;
        if (startFound && selectedCount < requestedCount) {
            endByte = nextOffset;
            ++selectedCount;
        }

        offset = nextOffset;
        ++codePointIndex;
    }

    // 선택 구간과 관계없이 input 전체를 먼저 decode해 잘못된 UTF-8에서
    // destination이 부분적으로 바뀌는 일을 막는다.
    if (!startFound) {
        if (firstCodePoint != codePointIndex) {
            return failure(Status::out_of_range);
        }

        startByte = input.size();
        endByte = input.size();
    }

    // 두 offset은 input.size() 안에서 순서가 보장되므로 뺄셈이 underflow하지 않는다.
    const std::size_t required = endByte - startByte;
    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    for (std::size_t index = 0; index < required; ++index) {
        output.data()[index] = input[startByte + index];
    }

    const Status commitStatus = output.commit(required);
    if (commitStatus != Status::ok) {
        return failure(commitStatus);
    }

    return {Status::ok, required, required};
}

WriteResult sanitize(StringView input, StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }

    std::size_t required = 0;
    std::size_t offset = 0;
    // 첫 pass에서 replacement 확장까지 포함한 전체 크기를 계산한다.
    while (offset < input.size()) {
        const DecodeResult decoded = detail::utf8::decodeNext(input, offset);
        const std::size_t increment =
            decoded.status == Status::ok ? decoded.bytes : sizeof(replacement);

        if (additionOverflows(required, increment)) {
            return failure(Status::out_of_range);
        }

        required += increment;
        offset += decoded.bytes;
    }

    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    offset = 0;
    std::size_t writeOffset = 0;
    // 공간이 충분한 경우에만 두 번째 pass에서 destination을 채운다.
    while (offset < input.size()) {
        const DecodeResult decoded = detail::utf8::decodeNext(input, offset);
        if (decoded.status == Status::ok) {
            for (std::size_t index = 0; index < decoded.bytes; ++index) {
                output.data()[writeOffset + index] = input[offset + index];
            }
            writeOffset += decoded.bytes;
        } else {
            for (std::size_t index = 0; index < sizeof(replacement); ++index) {
                output.data()[writeOffset + index] = replacement[index];
            }
            writeOffset += sizeof(replacement);
        }

        offset += decoded.bytes;
    }

    const Status commitStatus = output.commit(writeOffset);
    if (commitStatus != Status::ok) {
        return failure(commitStatus);
    }

    return {Status::ok, writeOffset, required};
}

} // namespace utf8
} // namespace util
} // namespace cms
