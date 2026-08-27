#include <cstddef>
#include <type_traits>

#include <cms/util/string_ops.h>

static_assert(
    std::is_same<decltype(cms::util::string::npos), const std::size_t>::value,
    "npos has the wrong type");

using Compare = int (*)(cms::util::StringView, cms::util::StringView) noexcept;
using Predicate = bool (*)(cms::util::StringView, cms::util::StringView) noexcept;
using Find = std::size_t (*)(
    cms::util::StringView,
    cms::util::StringView,
    std::size_t) noexcept;
using FindLast = std::size_t (*)(cms::util::StringView, cms::util::StringView) noexcept;
using Trim = cms::util::StringView (*)(cms::util::StringView) noexcept;
using Write = cms::util::WriteResult (*)(cms::util::StringView, cms::util::StringBuffer) noexcept;
using ReplaceAll = cms::util::WriteResult (*)(
    cms::util::StringView,
    cms::util::StringView,
    cms::util::StringView,
    cms::util::StringBuffer) noexcept;

static_assert(
    std::is_same<decltype(&cms::util::string::compare), Compare>::value,
    "compare has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::equals), Predicate>::value,
    "equals has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::startsWith), Predicate>::value,
    "startsWith has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::endsWith), Predicate>::value,
    "endsWith has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::find), Find>::value,
    "find has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::findLast), FindLast>::value,
    "findLast has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::trimAsciiWhitespace),
    Trim>::value,
    "trimAsciiWhitespace has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::compareIgnoreAsciiCase),
    Compare>::value,
    "compareIgnoreAsciiCase has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::equalsIgnoreAsciiCase),
    Predicate>::value,
    "equalsIgnoreAsciiCase has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::startsWithIgnoreAsciiCase),
    Predicate>::value,
    "startsWithIgnoreAsciiCase has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::endsWithIgnoreAsciiCase),
    Predicate>::value,
    "endsWithIgnoreAsciiCase has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::findIgnoreAsciiCase),
    Find>::value,
    "findIgnoreAsciiCase has the wrong signature");
static_assert(std::is_same<
    decltype(&cms::util::string::findLastIgnoreAsciiCase),
    FindLast>::value,
    "findLastIgnoreAsciiCase has the wrong signature");

using SplitTokens = cms::util::StringView[2];
static_assert(std::is_same<
    decltype(cms::util::string::split(
        cms::util::StringView(),
        ':',
        std::declval<SplitTokens&>())),
    std::size_t>::value,
    "split has the wrong return type");
static_assert(
    std::is_same<decltype(&cms::util::string::copy), Write>::value,
    "copy has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::copyTruncated), Write>::value,
    "copyTruncated has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::append), Write>::value,
    "append has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::appendTruncated), Write>::value,
    "appendTruncated has the wrong signature");
static_assert(
    std::is_same<decltype(&cms::util::string::replaceAll), ReplaceAll>::value,
    "replaceAll has the wrong signature");
