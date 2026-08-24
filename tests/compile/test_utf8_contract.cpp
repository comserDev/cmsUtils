#include <cstddef>
#include <type_traits>

#include <cms/utf8.h>

static_assert(
    std::is_same<decltype(cms::utf8::DecodeResult::status), cms::Status>::value,
    "DecodeResult::status has the wrong type");
static_assert(
    std::is_same<decltype(cms::utf8::DecodeResult::codePoint), char32_t>::value,
    "DecodeResult::codePoint has the wrong type");
static_assert(
    std::is_same<decltype(cms::utf8::DecodeResult::bytes), std::size_t>::value,
    "DecodeResult::bytes has the wrong type");
static_assert(
    std::is_copy_constructible<cms::utf8::DecodeResult>::value,
    "DecodeResult must be copy constructible");
static_assert(
    std::is_copy_assignable<cms::utf8::DecodeResult>::value,
    "DecodeResult must be copy assignable");
static_assert(
    std::is_move_constructible<cms::utf8::DecodeResult>::value,
    "DecodeResult must be move constructible");
static_assert(
    std::is_move_assignable<cms::utf8::DecodeResult>::value,
    "DecodeResult must be move assignable");
static_assert(
    std::is_trivially_copyable<cms::utf8::DecodeResult>::value,
    "DecodeResult must be trivially copyable");
static_assert(
    std::is_standard_layout<cms::utf8::DecodeResult>::value,
    "DecodeResult must have standard layout");

using DecodeNext = cms::utf8::DecodeResult (*)(
    cms::StringView,
    std::size_t) noexcept;
using Validate = cms::Status (*)(cms::StringView) noexcept;
using Count = cms::ParseResult<std::size_t> (*)(cms::StringView) noexcept;
using Substring = cms::WriteResult (*)(
    cms::StringView,
    std::size_t,
    std::size_t,
    cms::StringBuffer) noexcept;
using Sanitize = cms::WriteResult (*)(
    cms::StringView,
    cms::StringBuffer) noexcept;

static_assert(
    std::is_same<decltype(&cms::utf8::decodeNext), DecodeNext>::value,
    "decodeNext has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::utf8::validate), Validate>::value,
    "validate has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::utf8::count), Count>::value,
    "count has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::utf8::substring), Substring>::value,
    "substring has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::utf8::sanitize), Sanitize>::value,
    "sanitize has the wrong signature");
