#include <cms/util/format.h>

using FormatResult = decltype(cms::util::format::unsignedInteger(
    std::uint64_t{0},
    cms::util::StringBuffer{}));

static_assert(
    sizeof(FormatResult) == sizeof(cms::util::WriteResult),
    "format.h must compile independently");
