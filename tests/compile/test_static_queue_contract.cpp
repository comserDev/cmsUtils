#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <cms/util/detail/small_index.h>
#include <cms/util/static_queue.h>

namespace {

struct NonDefault {
    NonDefault() = delete;
    explicit NonDefault(int) noexcept {}
    ~NonDefault() noexcept = default;
};

struct NothrowElement {
    explicit NothrowElement(int) noexcept {}
    NothrowElement(const NothrowElement&) noexcept = default;
    NothrowElement(NothrowElement&&) noexcept = default;
    ~NothrowElement() noexcept = default;
};

struct ThrowingElement {
    explicit ThrowingElement(int) noexcept(false) {}
    ThrowingElement(const ThrowingElement&) noexcept(false) {}
    ThrowingElement(ThrowingElement&&) noexcept(false) {}
    ~ThrowingElement() noexcept = default;
};

using Queue = cms::util::StaticQueue<NothrowElement, 4>;

} // namespace

static_assert(
    std::is_default_constructible<cms::util::StaticQueue<NonDefault, 1>>::value,
    "StaticQueue must support non-default-constructible elements");
static_assert(!std::is_copy_constructible<Queue>::value, "Queue copy is deleted");
static_assert(!std::is_copy_assignable<Queue>::value, "Queue copy assign is deleted");
static_assert(!std::is_move_constructible<Queue>::value, "Queue move is deleted");
static_assert(!std::is_move_assignable<Queue>::value, "Queue move assign is deleted");

static_assert(
    std::is_same<
        decltype(std::declval<const Queue&>().size()),
        std::size_t>::value,
    "size has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<const Queue&>().capacity()),
        std::size_t>::value,
    "capacity has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const Queue&>().empty()), bool>::value,
    "empty has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<const Queue&>().full()), bool>::value,
    "full has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<Queue&>().front()), NothrowElement*>::value,
    "mutable front has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<const Queue&>().front()),
        const NothrowElement*>::value,
    "const front has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<Queue&>().pop()), cms::util::Status>::value,
    "pop has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<Queue&>().clear()), void>::value,
    "clear has the wrong return type");
static_assert(
    std::is_same<decltype(std::declval<Queue&>().emplace(1)), cms::util::Status>::value,
    "emplace has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<Queue&>().push(
            std::declval<const NothrowElement&>())),
        cms::util::Status>::value,
    "copy push has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<Queue&>().push(
            std::declval<NothrowElement&&>())),
        cms::util::Status>::value,
    "move push has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<Queue&>().pushOverwrite(
            std::declval<const NothrowElement&>())),
        cms::util::Status>::value,
    "copy overwrite push has the wrong return type");
static_assert(
    std::is_same<
        decltype(std::declval<Queue&>().pushOverwrite(
            std::declval<NothrowElement&&>())),
        cms::util::Status>::value,
    "move overwrite push has the wrong return type");

static_assert(
    std::is_nothrow_default_constructible<Queue>::value,
    "queue default constructor must be noexcept");
static_assert(noexcept(std::declval<const Queue&>().size()), "size must be noexcept");
static_assert(
    noexcept(std::declval<const Queue&>().capacity()),
    "capacity must be noexcept");
static_assert(noexcept(std::declval<const Queue&>().empty()), "empty must be noexcept");
static_assert(noexcept(std::declval<const Queue&>().full()), "full must be noexcept");
static_assert(noexcept(std::declval<Queue&>().front()), "mutable front must be noexcept");
static_assert(
    noexcept(std::declval<const Queue&>().front()),
    "const front must be noexcept");
static_assert(noexcept(std::declval<Queue&>().push(
    std::declval<const NothrowElement&>())), "nothrow copy push expected");
static_assert(noexcept(std::declval<Queue&>().push(
    std::declval<NothrowElement&&>())), "nothrow move push expected");
static_assert(noexcept(std::declval<Queue&>().pushOverwrite(
    std::declval<const NothrowElement&>())),
    "copy overwrite push must be noexcept");
static_assert(noexcept(std::declval<Queue&>().pushOverwrite(
    std::declval<NothrowElement&&>())),
    "move overwrite push must be noexcept");
static_assert(noexcept(std::declval<Queue&>().emplace(1)), "nothrow emplace expected");
static_assert(noexcept(std::declval<Queue&>().pop()), "pop must be noexcept");
static_assert(noexcept(std::declval<Queue&>().clear()), "clear must be noexcept");
static_assert(std::is_nothrow_destructible<Queue>::value, "queue destructor must be noexcept");

using ThrowingQueue = cms::util::StaticQueue<ThrowingElement, 2>;
static_assert(!noexcept(std::declval<ThrowingQueue&>().push(
    std::declval<const ThrowingElement&>())), "throwing copy push expected");
static_assert(!noexcept(std::declval<ThrowingQueue&>().push(
    std::declval<ThrowingElement&&>())), "throwing move push expected");
static_assert(
    !noexcept(std::declval<ThrowingQueue&>().emplace(1)),
    "throwing emplace expected");

#if SIZE_MAX >= 255
static_assert(
    std::is_same<cms::util::detail::small_index_t<255>, std::uint8_t>::value,
    "255 must fit uint8_t");
#endif

#if SIZE_MAX >= 256
static_assert(
    std::is_same<cms::util::detail::small_index_t<256>, std::uint16_t>::value,
    "256 requires uint16_t");
#endif

#if SIZE_MAX >= 65535
static_assert(
    std::is_same<cms::util::detail::small_index_t<65535>, std::uint16_t>::value,
    "65535 must fit uint16_t");
#endif

#if SIZE_MAX >= 65536
static_assert(
    std::is_same<cms::util::detail::small_index_t<65536>, std::uint32_t>::value,
    "65536 requires uint32_t");
#endif
