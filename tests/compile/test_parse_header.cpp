#include <cms/parse.h>

using ParseResult = decltype(cms::parse::unsignedInteger(cms::StringView{}));

static_assert(
    sizeof(ParseResult) == sizeof(cms::ParseResult<std::uint64_t>),
    "parse.h must compile independently");
