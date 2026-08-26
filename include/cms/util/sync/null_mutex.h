#pragma once

namespace cms {
namespace util {
namespace sync {

// 동기화가 필요 없는 환경에서 같은 API를 유지하기 위한 no-op mutex다.
class NullMutex {
public:
    void lock() noexcept {}
    void unlock() noexcept {}
};

} // namespace sync
} // namespace util
} // namespace cms
