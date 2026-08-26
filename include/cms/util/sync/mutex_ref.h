#pragma once

#include <utility>

namespace cms {
namespace util {
namespace sync {

// 외부 mutex를 alias한다. wrapper보다 외부 mutex가 오래 살아야 한다.
template<class Mutex>
class MutexRef {
public:
    explicit MutexRef(Mutex& mutex) noexcept
        : mutex_(&mutex) {}

    void lock()
        noexcept(noexcept(std::declval<Mutex&>().lock())) {
        mutex_->lock();
    }

    void unlock()
        noexcept(noexcept(std::declval<Mutex&>().unlock())) {
        mutex_->unlock();
    }

private:
    Mutex* mutex_;
};

} // namespace sync
} // namespace util
} // namespace cms
