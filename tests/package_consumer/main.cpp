#include <cms/util/log/async_logger.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/status.h>
#include <cms/util/sync/null_mutex.h>

namespace {

class FixedClock {
public:
    cms::util::log::Timestamp nowMilliseconds() noexcept {
        return 1;
    }
};

} // namespace

int main() {
    cms::util::StaticString<16> message;
    if (message.assign("installed").status != cms::util::Status::ok) {
        return 1;
    }

    cms::util::StaticQueue<int, 2> queue;
    if (queue.push(7) != cms::util::Status::ok || queue.front() == nullptr) {
        return 2;
    }

    using Logger = cms::util::log::AsyncLogger<
        32,
        2,
        FixedClock,
        cms::util::platform::StdoutSink,
        cms::util::sync::NullMutex>;

    Logger logger{FixedClock{}, cms::util::platform::StdoutSink{}};
    if (logger.log(cms::util::log::Level::info, message.view())
        != cms::util::Status::ok) {
        return 3;
    }

    return logger.drainOne() == cms::util::Status::ok ? 0 : 4;
}
