#include <cstdint>
#include <type_traits>

#include <cms/util/format.h>

using UnsignedFormat = cms::util::WriteResult (*)(
    std::uint64_t,
    cms::util::StringBuffer,
    unsigned int,
    bool) noexcept;
using SignedFormat = cms::util::WriteResult (*)(
    std::int64_t,
    cms::util::StringBuffer,
    unsigned int,
    bool) noexcept;

static_assert(
    std::is_same<
        decltype(&cms::util::format::unsignedInteger),
        UnsignedFormat>::value,
    "unsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::util::format::signedInteger),
        SignedFormat>::value,
    "signedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::util::format::appendUnsignedInteger),
        UnsignedFormat>::value,
    "appendUnsignedInteger has the wrong signature");
static_assert(
    std::is_same<
        decltype(&cms::util::format::appendSignedInteger),
        SignedFormat>::value,
    "appendSignedInteger has the wrong signature");
