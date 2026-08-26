#include <cms/util/parse.h>

using ParseResult = decltype(cms::util::parse::unsignedInteger(cms::util::StringView{}));

static_assert(
    sizeof(ParseResult) == sizeof(cms::util::ParseResult<std::uint64_t>),
    "parse.h must compile independently");
