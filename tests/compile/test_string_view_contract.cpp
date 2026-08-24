#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/string_view.h>

static_assert(std::is_copy_constructible<cms::StringView>::value, "StringView must be copyable");
static_assert(std::is_copy_assignable<cms::StringView>::value, "StringView must be copy assignable");
static_assert(std::is_move_constructible<cms::StringView>::value, "StringView must be movable");
static_assert(std::is_move_assignable<cms::StringView>::value, "StringView must be move assignable");
static_assert(std::is_trivially_copyable<cms::StringView>::value, "StringView must be trivial to copy");
static_assert(std::is_standard_layout<cms::StringView>::value, "StringView must have standard layout");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::StringView&>().data()),
        const char*>::value,
    "StringView::data must preserve const access");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::StringView&>().size()),
        std::size_t>::value,
    "StringView::size has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::StringView&>()[0]),
        char>::value,
    "StringView::operator[] has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::StringView&>().substr(0, 0)),
        cms::StringView>::value,
    "StringView::substr has the wrong type");
