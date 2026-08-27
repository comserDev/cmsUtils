#include <cstdint>
#include <type_traits>

#include <cms/util/parse.h>

using UnsignedParse = cms::util::ParseResult<std::uint64_t> (*)(
    cms::util::StringView,
    unsigned int) noexcept;
using SignedParse = cms::util::ParseResult<std::int64_t> (*)(
    cms::util::StringView,
    unsigned int) noexcept;
using FloatingParse = cms::util::ParseResult<double> (*)(
    cms::util::StringView) noexcept;

static_assert(
    std::is_same<
        decltype(&cms::util::parse::unsignedInteger),
        UnsignedParse>::value,
    "unsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::util::parse::signedInteger),
        SignedParse>::value,
    "signedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::util::parse::floatingPoint),
        FloatingParse>::value,
    "floatingPoint has the wrong signature");
