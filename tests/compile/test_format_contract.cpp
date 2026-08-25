#include <cstdint>
#include <type_traits>

#include <cms/format.h>

using UnsignedFormat = cms::WriteResult (*)(
    std::uint64_t,
    cms::StringBuffer,
    unsigned int,
    bool) noexcept;
using SignedFormat = cms::WriteResult (*)(
    std::int64_t,
    cms::StringBuffer,
    unsigned int,
    bool) noexcept;

static_assert(
    std::is_same<
        decltype(&cms::format::unsignedInteger),
        UnsignedFormat>::value,
    "unsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::format::signedInteger),
        SignedFormat>::value,
    "signedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::format::appendUnsignedInteger),
        UnsignedFormat>::value,
    "appendUnsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::format::appendSignedInteger),
        SignedFormat>::value,
    "appendSignedInteger has the wrong signature");
