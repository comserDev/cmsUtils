#include <cstddef>
#include <type_traits>
#include <utility>

#include <cms/util/string_buffer.h>

static_assert(std::is_copy_constructible<cms::util::StringBuffer>::value, "StringBuffer must be copyable");
static_assert(std::is_copy_assignable<cms::util::StringBuffer>::value, "StringBuffer must be copy assignable");
static_assert(std::is_move_constructible<cms::util::StringBuffer>::value, "StringBuffer must be movable");
static_assert(std::is_move_assignable<cms::util::StringBuffer>::value, "StringBuffer must be move assignable");
static_assert(std::is_trivially_copyable<cms::util::StringBuffer>::value, "StringBuffer must be trivial to copy");
static_assert(std::is_standard_layout<cms::util::StringBuffer>::value, "StringBuffer must have standard layout");
static_assert(
    std::is_constructible<
        cms::util::StringBuffer,
        char*,
        std::size_t,
        std::size_t&>::value,
    "StringBuffer must borrow an external size state");
static_assert(
    !std::is_constructible<
        cms::util::StringBuffer,
        char*,
        std::size_t,
        std::size_t*>::value,
    "StringBuffer must not accept a nullable size state");
static_assert(
    std::is_same<
        decltype(std::declval<cms::util::StringBuffer&>().data()),
        char*>::value,
    "Mutable StringBuffer::data has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::util::StringBuffer&>().data()),
        const char*>::value,
    "Const StringBuffer::data has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<const cms::util::StringBuffer&>().view()),
        cms::util::StringView>::value,
    "StringBuffer::view has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<cms::util::StringBuffer&>().clear()),
        cms::util::Status>::value,
    "StringBuffer::clear has the wrong type");
static_assert(
    std::is_same<
        decltype(std::declval<cms::util::StringBuffer&>().commit(0)),
        cms::util::Status>::value,
    "StringBuffer::commit has the wrong type");
