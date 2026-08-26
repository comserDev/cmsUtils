#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/util/static_string.h>

using String = cms::util::StaticString<8>;

static_assert(std::is_default_constructible<String>::value, "Must default construct");
static_assert(std::is_copy_constructible<String>::value, "Must copy construct");
static_assert(std::is_copy_assignable<String>::value, "Must copy assign");
static_assert(std::is_move_constructible<String>::value, "Must move construct");
static_assert(std::is_move_assignable<String>::value, "Must move assign");
static_assert(std::is_standard_layout<String>::value, "Must have standard layout");
static_assert(
    !std::is_convertible<String, const char*>::value,
    "StaticString must not implicitly convert to const char*");
static_assert(
    !std::is_constructible<String, cms::util::StringView>::value,
    "StringView construction is intentionally unsupported");
static_assert(
    !std::is_constructible<cms::util::StaticString<16>, const String&>::value,
    "Cross-capacity copy construction is intentionally unsupported");
static_assert(
    !std::is_assignable<cms::util::StaticString<16>&, const String&>::value,
    "Cross-capacity copy assignment is intentionally unsupported");
static_assert(sizeof(cms::util::StaticString<1>) > 0, "StorageBytes == 1 must compile");

static_assert(
    std::is_same<
        decltype(std::declval<const String&>().cStr()),
        const char*>::value,
    "cStr has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<String&>().data()),
        const char*>::value,
    "data must not expose mutable raw storage");
static_assert(
    std::is_same<decltype(std::declval<const String&>().size()), std::size_t>::value,
    "size has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const String&>().capacity()), std::size_t>::value,
    "capacity has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const String&>().maxSize()), std::size_t>::value,
    "maxSize has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const String&>().remaining()), std::size_t>::value,
    "remaining has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const String&>().empty()), bool>::value,
    "empty has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const String&>().view()), cms::util::StringView>::value,
    "view has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<String&>().buffer()), cms::util::StringBuffer>::value,
    "buffer has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<String&>().clear()), void>::value,
    "clear has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<String&>().assign(std::declval<cms::util::StringView>())),
        cms::util::WriteResult>::value,
    "assign has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<String&>().append(std::declval<cms::util::StringView>())),
        cms::util::WriteResult>::value,
    "append has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<String&>().assignTruncated(
            std::declval<cms::util::StringView>())),
        cms::util::WriteResult>::value,
    "assignTruncated has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<String&>().appendTruncated(
            std::declval<cms::util::StringView>())),
        cms::util::WriteResult>::value,
    "appendTruncated has the wrong return type");

static_assert(noexcept(std::declval<String&>().clear()), "clear must be noexcept");
static_assert(noexcept(std::declval<String&>().buffer()), "buffer must be noexcept");
static_assert(
    noexcept(std::declval<String&>().assign(std::declval<cms::util::StringView>())),
    "assign must be noexcept");
static_assert(
    noexcept(std::declval<String&>().append(std::declval<cms::util::StringView>())),
    "append must be noexcept");
static_assert(
    noexcept(std::declval<String&>().assignTruncated(
        std::declval<cms::util::StringView>())),
    "assignTruncated must be noexcept");
static_assert(
    noexcept(std::declval<String&>().appendTruncated(
        std::declval<cms::util::StringView>())),
    "appendTruncated must be noexcept");
