#include <cstdint>
#include <iostream>

#include <cms/util/log/async_logger.h>
#include <cms/util/parse.h>
#include <cms/util/platform/stdout_sink.h>
#include <cms/util/platform/system_clock.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/string_ops.h>
#include <cms/util/sync/null_mutex.h>

namespace {

bool succeeded(cms::util::Status status, const char* operation) {
    if (status == cms::util::Status::ok) {
        return true;
    }
    std::cerr << operation << " failed\n";
    return false;
}

} // namespace

int main() {
    cms::util::StaticString<32> text;
    if (!succeeded(text.assign("device=").status, "assign")
        || !succeeded(text.append("cmsUtils").status, "append")) {
        return 1;
    }
    std::cout << text.cStr() << '\n';

    cms::util::StringView tokens[2];
    const cms::util::StringView setting("port:8080");
    const std::size_t tokenCount =
        cms::util::string::split(setting, ':', tokens);
    if (tokenCount != 2) {
        return 1;
    }
    const auto port = cms::util::parse::unsignedInteger(tokens[1]);
    if (port.status != cms::util::Status::ok
        || port.consumed != tokens[1].size()) {
        return 1;
    }
    std::cout << "port=" << port.value << '\n';

    cms::util::StaticQueue<int, 2> queue;
    if (!succeeded(queue.push(10), "queue.push")
        || !succeeded(queue.push(20), "queue.push")) {
        return 1;
    }
    if (queue.push(30) != cms::util::Status::no_space
        || !succeeded(queue.pushOverwrite(30), "queue.pushOverwrite")) {
        return 1;
    }
    const int* const front = queue.front();
    if (front == nullptr) {
        return 1;
    }
    std::cout << "queue front=" << *front << '\n';
    if (!succeeded(queue.pop(), "queue.pop")) {
        return 1;
    }

    using Logger = cms::util::log::AsyncLogger<
        64,
        4,
        cms::util::platform::SystemClock,
        cms::util::platform::StdoutSink,
        cms::util::sync::NullMutex>;
    Logger logger{
        cms::util::platform::SystemClock{},
        cms::util::platform::StdoutSink{}};
    if (!succeeded(
            logger.log(cms::util::log::Level::info, "native example ready"),
            "logger.log")
        || !succeeded(logger.drainOne(), "logger.drainOne")) {
        return 1;
    }

    return 0;
}
