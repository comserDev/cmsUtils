#include <type_traits>
#include <utility>

#include <cms/util/platform/std_file_sink.h>

static_assert(
    !std::is_copy_constructible<cms::util::platform::StdFileSink>::value,
    "StdFileSink copy must be deleted");
static_assert(
    std::is_nothrow_move_constructible<
        cms::util::platform::StdFileSink>::value,
    "StdFileSink move must transfer ownership without throwing");
static_assert(
    !std::is_move_assignable<cms::util::platform::StdFileSink>::value,
    "StdFileSink move assignment must be deleted");
static_assert(std::is_same<
    decltype(std::declval<cms::util::platform::StdFileSink&>().write(
        cms::util::StringView())),
    cms::util::Status>::value,
    "StdFileSink write must return Status");
