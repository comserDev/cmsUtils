#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <cms/util/status.h>

static_assert(std::is_enum<cms::util::Status>::value, "Status must be an enum");
static_assert(
    std::is_same<
        typename std::underlying_type<cms::util::Status>::type,
        std::uint8_t>::value,
    "Status must use uint8_t as its underlying type");
static_assert(
    !std::is_convertible<cms::util::Status, int>::value,
    "Status must retain scoped enum semantics");
static_assert(std::is_copy_constructible<cms::util::Status>::value, "Status must be copyable");
static_assert(std::is_move_constructible<cms::util::Status>::value, "Status must be movable");
static_assert(std::is_trivially_copyable<cms::util::Status>::value, "Status must be trivial to copy");

static_assert(std::is_aggregate<cms::util::WriteResult>::value, "WriteResult must be an aggregate");
static_assert(std::is_copy_constructible<cms::util::WriteResult>::value, "WriteResult must be copyable");
static_assert(std::is_move_constructible<cms::util::WriteResult>::value, "WriteResult must be movable");
static_assert(std::is_trivially_copyable<cms::util::WriteResult>::value, "WriteResult must be trivial to copy");
static_assert(std::is_standard_layout<cms::util::WriteResult>::value, "WriteResult must have standard layout");
static_assert(
    std::is_same<decltype(cms::util::WriteResult::status), cms::util::Status>::value,
    "WriteResult::status has the wrong type");
static_assert(
    std::is_same<decltype(cms::util::WriteResult::written), std::size_t>::value,
    "WriteResult::written has the wrong type");
static_assert(
    std::is_same<decltype(cms::util::WriteResult::required), std::size_t>::value,
    "WriteResult::required has the wrong type");

using IntParseResult = cms::util::ParseResult<int>;

static_assert(std::is_aggregate<IntParseResult>::value, "ParseResult must be an aggregate");
static_assert(std::is_copy_constructible<IntParseResult>::value, "ParseResult must be copyable");
static_assert(std::is_move_constructible<IntParseResult>::value, "ParseResult must be movable");
static_assert(std::is_trivially_copyable<IntParseResult>::value, "ParseResult must be trivial to copy");
static_assert(std::is_standard_layout<IntParseResult>::value, "ParseResult must have standard layout");
static_assert(
    std::is_same<decltype(IntParseResult::status), cms::util::Status>::value,
    "ParseResult::status has the wrong type");
static_assert(
    std::is_same<decltype(IntParseResult::value), int>::value,
    "ParseResult::value has the wrong type");
static_assert(
    std::is_same<decltype(IntParseResult::consumed), std::size_t>::value,
    "ParseResult::consumed has the wrong type");

constexpr IntParseResult valueInitialized{cms::util::Status::invalid_argument};
static_assert(valueInitialized.value == 0, "ParseResult::value must be value-initialized");
static_assert(valueInitialized.consumed == 0, "Omitted aggregate fields must be zero-initialized");
