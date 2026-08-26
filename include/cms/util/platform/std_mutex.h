#pragma once

#include <mutex>

namespace cms {
namespace util {
namespace platform {

// Desktop/host용 adapter이며 std::mutex 내부 allocation 여부는 보장하지 않는다.
class StdMutex {
public:
    StdMutex() = default;

    StdMutex(const StdMutex&) = delete;
    StdMutex& operator=(const StdMutex&) = delete;
    StdMutex(StdMutex&&) = delete;
    StdMutex& operator=(StdMutex&&) = delete;

    void lock() noexcept(noexcept(mutex_.lock())) {
        mutex_.lock();
    }

    void unlock() noexcept(noexcept(mutex_.unlock())) {
        mutex_.unlock();
    }

private:
    std::mutex mutex_;
};

} // namespace platform
} // namespace util
} // namespace cms
