#include <cstdint>
#include <type_traits>

#include <cms/parse.h>

using UnsignedParse = cms::ParseResult<std::uint64_t> (*)(
    cms::StringView,
    unsigned int) noexcept;
using SignedParse = cms::ParseResult<std::int64_t> (*)(
    cms::StringView,
    unsigned int) noexcept;

static_assert(
    std::is_same<
        decltype(&cms::parse::unsignedInteger),
        UnsignedParse>::value,
    "unsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::parse::signedInteger),
        SignedParse>::value,
    "signedInteger has the wrong signature");
