#pragma once

#include <utility>

namespace cms {
namespace sync {

// mutex를 소유하지 않으므로 guard보다 mutex가 오래 살아야 한다.
template<class Mutex>
class LockGuard {
public:
    explicit LockGuard(Mutex& mutex)
        noexcept(noexcept(mutex.lock()))
        : mutex_(mutex) {
        mutex_.lock();
    }

    ~LockGuard()
        noexcept(noexcept(std::declval<Mutex&>().unlock())) {
        mutex_.unlock();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
    LockGuard(LockGuard&&) = delete;
    LockGuard& operator=(LockGuard&&) = delete;

private:
    Mutex& mutex_;
};

} // namespace sync
} // namespace cms
