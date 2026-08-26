#include <cstddef>
#include <type_traits>

#include <cms/util/utf8.h>

static_assert(
    std::is_same<decltype(cms::util::utf8::DecodeResult::status), cms::util::Status>::value,
    "DecodeResult::status has the wrong type");
static_assert(
    std::is_same<decltype(cms::util::utf8::DecodeResult::codePoint), char32_t>::value,
    "DecodeResult::codePoint has the wrong type");
static_assert(
    std::is_same<decltype(cms::util::utf8::DecodeResult::bytes), std::size_t>::value,
    "DecodeResult::bytes has the wrong type");
static_assert(
    std::is_copy_constructible<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must be copy constructible");
static_assert(
    std::is_copy_assignable<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must be copy assignable");
static_assert(
    std::is_move_constructible<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must be move constructible");
static_assert(
    std::is_move_assignable<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must be move assignable");
static_assert(
    std::is_trivially_copyable<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must be trivially copyable");
static_assert(
    std::is_standard_layout<cms::util::utf8::DecodeResult>::value,
    "DecodeResult must have standard layout");

using DecodeNext = cms::util::utf8::DecodeResult (*)(
    cms::util::StringView,
    std::size_t) noexcept;
using Validate = cms::util::Status (*)(cms::util::StringView) noexcept;
using Count = cms::util::ParseResult<std::size_t> (*)(cms::util::StringView) noexcept;
using Substring = cms::util::WriteResult (*)(
    cms::util::StringView,
    std::size_t,
    std::size_t,
    cms::util::StringBuffer) noexcept;
using Sanitize = cms::util::WriteResult (*)(
    cms::util::StringView,
    cms::util::StringBuffer) noexcept;

static_assert(
    std::is_same<decltype(&cms::util::utf8::decodeNext), DecodeNext>::value,
    "decodeNext has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::utf8::validate), Validate>::value,
    "validate has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::utf8::count), Count>::value,
    "count has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::utf8::substring), Substring>::value,
    "substring has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::utf8::sanitize), Sanitize>::value,
    "sanitize has the wrong signature");
