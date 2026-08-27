#include <type_traits>
#include <utility>

#include <cms/util/log/tee_sink.h>

namespace {

struct HeaderSink {
    cms::util::Status write(cms::util::StringView) noexcept {
        return cms::util::Status::ok;
    }
};

using HeaderTee = cms::util::log::TeeSink<HeaderSink, HeaderSink>;

static_assert(std::is_same<
    decltype(std::declval<HeaderTee&>().write(cms::util::StringView())),
    cms::util::Status>::value,
    "TeeSink write must return Status");

} // namespace
