#pragma once

#include <cstddef>

#include <cms/util/log/clock.h>
#include <cms/util/log/level.h>
#include <cms/util/static_string.h>
#include <cms/util/status.h>
#include <cms/util/string_view.h>

namespace cms {
namespace util {
namespace log {

// message를 소유하지 않으므로 Record를 사용하는 동안 caller가 lifetime을 보장한다.
struct Record {
    Level level;
    Timestamp timestampMilliseconds;
    StringView message;
};

template<std::size_t MessageBytes>
// message를 고정 storage에 복사해 보관하는 zero-heap record다. assign이 실패하면
// level과 timestamp를 포함한 기존 record 상태를 유지한다.
class StaticRecord {
    static_assert(
        MessageBytes > 0,
        "StaticRecord requires at least one message storage byte");

public:
    constexpr StaticRecord() noexcept
        : level_(Level::info), timestampMilliseconds_(0), message_() {}

    constexpr std::size_t messageCapacity() const noexcept {
        return MessageBytes;
    }

    constexpr std::size_t maxMessageSize() const noexcept {
        return MessageBytes - 1;
    }

    Level level() const noexcept {
        return level_;
    }

    Timestamp timestampMilliseconds() const noexcept {
        return timestampMilliseconds_;
    }

    StringView message() const noexcept {
        return message_.view();
    }

    Record view() const noexcept {
        return {level_, timestampMilliseconds_, message_.view()};
    }

    // message를 복사한 뒤 성공할 때만 metadata와 함께 새 record를 publish한다.
    WriteResult assign(
        Level level,
        Timestamp timestampMilliseconds,
        StringView message) noexcept {
        const WriteResult result = message_.assign(message);
        if (result.status != Status::ok) {
            return result;
        }

        level_ = level;
        timestampMilliseconds_ = timestampMilliseconds;
        return result;
    }

private:
    Level level_;
    Timestamp timestampMilliseconds_;
    StaticString<MessageBytes> message_;
};

} // namespace log
} // namespace util
} // namespace cms
