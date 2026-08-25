#include <cms/string_ops.h>

#include <cstring>
#include <limits>

namespace cms {
namespace string {

namespace {

unsigned char byteAt(StringView value, std::size_t index) noexcept {
    return static_cast<unsigned char>(value[index]);
}

bool bytesEqual(
    StringView lhs,
    std::size_t lhsOffset,
    StringView rhs) noexcept {
    for (std::size_t index = 0; index < rhs.size(); ++index) {
        if (lhs[lhsOffset + index] != rhs[index]) {
            return false;
        }
    }
    return true;
}

void moveBytes(
    char* destination,
    const char* source,
    std::size_t count) noexcept {
    if (count > 0) {
        std::memmove(destination, source, count);
    }
}

bool addSize(
    std::size_t current,
    std::size_t increment,
    std::size_t& result) noexcept {
    if (increment > (std::numeric_limits<std::size_t>::max)() - current) {
        return false;
    }

    result = current + increment;
    return true;
}

WriteResult failure(Status status) noexcept {
    return {status, 0, 0};
}

} // namespace

int compare(StringView lhs, StringView rhs) noexcept {
    const std::size_t sharedSize =
        lhs.size() < rhs.size() ? lhs.size() : rhs.size();
    for (std::size_t index = 0; index < sharedSize; ++index) {
        const unsigned char left = byteAt(lhs, index);
        const unsigned char right = byteAt(rhs, index);
        if (left < right) {
            return -1;
        }
        if (left > right) {
            return 1;
        }
    }

    if (lhs.size() < rhs.size()) {
        return -1;
    }
    if (lhs.size() > rhs.size()) {
        return 1;
    }
    return 0;
}

bool equals(StringView lhs, StringView rhs) noexcept {
    return lhs.size() == rhs.size() && bytesEqual(lhs, 0, rhs);
}

bool startsWith(StringView value, StringView prefix) noexcept {
    return prefix.size() <= value.size() && bytesEqual(value, 0, prefix);
}

bool endsWith(StringView value, StringView suffix) noexcept {
    return suffix.size() <= value.size()
        && bytesEqual(value, value.size() - suffix.size(), suffix);
}

std::size_t find(
    StringView value,
    StringView needle,
    std::size_t start) noexcept {
    if (start > value.size()) {
        return npos;
    }
    if (needle.empty()) {
        return start;
    }
    if (needle.size() > value.size() - start) {
        return npos;
    }

    const std::size_t lastStart = value.size() - needle.size();
    std::size_t position = start;
    // lastStart에서 먼저 멈춰 SIZE_MAX까지 increment되는 경우를 만들지 않는다.
    while (true) {
        if (bytesEqual(value, position, needle)) {
            return position;
        }
        if (position == lastStart) {
            break;
        }
        ++position;
    }
    return npos;
}

std::size_t findLast(StringView value, StringView needle) noexcept {
    if (needle.empty()) {
        return value.size();
    }
    if (needle.size() > value.size()) {
        return npos;
    }

    std::size_t position = value.size() - needle.size();
    while (true) {
        if (bytesEqual(value, position, needle)) {
            return position;
        }
        if (position == 0) {
            break;
        }
        --position;
    }
    return npos;
}

WriteResult copy(StringView input, StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }
    if (input.size() > output.maxSize()) {
        return {Status::no_space, 0, input.size()};
    }

    moveBytes(output.data(), input.data(), input.size());
    const Status status = output.commit(input.size());
    if (status != Status::ok) {
        return failure(status);
    }
    return {Status::ok, input.size(), input.size()};
}

WriteResult copyTruncated(StringView input, StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }

    const std::size_t copySize =
        input.size() < output.maxSize() ? input.size() : output.maxSize();
    moveBytes(output.data(), input.data(), copySize);
    const Status commitStatus = output.commit(copySize);
    if (commitStatus != Status::ok) {
        return failure(commitStatus);
    }

    const Status status =
        copySize == input.size() ? Status::ok : Status::no_space;
    return {status, copySize, input.size()};
}

WriteResult append(StringView input, StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }
    if (input.size() > output.remaining()) {
        return {Status::no_space, 0, input.size()};
    }

    const std::size_t oldSize = output.size();
    moveBytes(output.data() + oldSize, input.data(), input.size());
    const Status status = output.commit(oldSize + input.size());
    if (status != Status::ok) {
        return failure(status);
    }
    return {Status::ok, input.size(), input.size()};
}

WriteResult appendTruncated(StringView input, StringBuffer output) noexcept {
    if (!output.valid()) {
        return failure(Status::invalid_argument);
    }

    const std::size_t available = output.remaining();
    const std::size_t copySize =
        input.size() < available ? input.size() : available;
    const std::size_t oldSize = output.size();
    moveBytes(output.data() + oldSize, input.data(), copySize);
    const Status commitStatus = output.commit(oldSize + copySize);
    if (commitStatus != Status::ok) {
        return failure(commitStatus);
    }

    const Status status =
        copySize == input.size() ? Status::ok : Status::no_space;
    return {status, copySize, input.size()};
}

WriteResult replaceAll(
    StringView input,
    StringView needle,
    StringView replacement,
    StringBuffer output) noexcept {
    if (!output.valid() || needle.empty()) {
        return failure(Status::invalid_argument);
    }

    std::size_t required = 0;
    std::size_t inputOffset = 0;
    // 첫 pass에서는 non-overlapping match를 세고 전체 결과 크기만 계산한다.
    // 이 단계가 끝날 때까지 output은 전혀 건드리지 않는다.
    while (inputOffset < input.size()) {
        const bool matches =
            needle.size() <= input.size() - inputOffset
            && bytesEqual(input, inputOffset, needle);
        const std::size_t increment = matches ? replacement.size() : 1;
        if (!addSize(required, increment, required)) {
            return failure(Status::out_of_range);
        }
        inputOffset += matches ? needle.size() : 1;
    }

    if (required > output.maxSize()) {
        return {Status::no_space, 0, required};
    }

    inputOffset = 0;
    std::size_t outputOffset = 0;
    // capacity가 충분하다는 것이 확인된 뒤 두 번째 pass에서 실제로 기록한다.
    while (inputOffset < input.size()) {
        const bool matches =
            needle.size() <= input.size() - inputOffset
            && bytesEqual(input, inputOffset, needle);
        if (matches) {
            moveBytes(
                output.data() + outputOffset,
                replacement.data(),
                replacement.size());
            outputOffset += replacement.size();
            inputOffset += needle.size();
        } else {
            output.data()[outputOffset] = input[inputOffset];
            ++outputOffset;
            ++inputOffset;
        }
    }

    const Status status = output.commit(outputOffset);
    if (status != Status::ok) {
        return failure(status);
    }
    return {Status::ok, outputOffset, required};
}

} // namespace string
} // namespace cms
