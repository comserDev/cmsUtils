#include <cstddef>
#include <type_traits>

#include <cms/string_ops.h>

static_assert(
    std::is_same<decltype(cms::string::npos), const std::size_t>::value,
    "npos has the wrong type");

using Compare = int (*)(cms::StringView, cms::StringView) noexcept;
using Predicate = bool (*)(cms::StringView, cms::StringView) noexcept;
using Find = std::size_t (*)(
    cms::StringView,
    cms::StringView,
    std::size_t) noexcept;
using FindLast = std::size_t (*)(cms::StringView, cms::StringView) noexcept;
using Write = cms::WriteResult (*)(cms::StringView, cms::StringBuffer) noexcept;
using ReplaceAll = cms::WriteResult (*)(
    cms::StringView,
    cms::StringView,
    cms::StringView,
    cms::StringBuffer) noexcept;

static_assert(
    std::is_same<decltype(&cms::string::compare), Compare>::value,
    "compare has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::equals), Predicate>::value,
    "equals has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::startsWith), Predicate>::value,
    "startsWith has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::endsWith), Predicate>::value,
    "endsWith has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::find), Find>::value,
    "find has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::findLast), FindLast>::value,
    "findLast has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::copy), Write>::value,
    "copy has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::copyTruncated), Write>::value,
    "copyTruncated has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::append), Write>::value,
    "append has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::appendTruncated), Write>::value,
    "appendTruncated has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::string::replaceAll), ReplaceAll>::value,
    "replaceAll has the wrong signature");
