#include <cms/format.h>

using FormatResult = decltype(cms::format::unsignedInteger(
    std::uint64_t{0},
    cms::StringBuffer{}));

static_assert(
    sizeof(FormatResult) == sizeof(cms::WriteResult),
    "format.h must compile independently");
