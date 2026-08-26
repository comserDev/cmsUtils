#pragma once

namespace cms {
namespace util {
namespace log {

// full이면 기존 FIFO를 유지하고 Status::no_space를 반환하는 기본 policy다.
struct RejectOnFull {};

// full이면 oldest element를 제거하고 새 element를 저장하는 opt-in policy다.
struct OverwriteOldestOnFull {};

} // namespace log
} // namespace util
} // namespace cms
