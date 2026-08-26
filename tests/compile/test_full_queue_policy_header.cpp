#include <type_traits>

#include <cms/log/full_queue_policy.h>

static_assert(std::is_empty<cms::log::RejectOnFull>::value,
    "RejectOnFull must remain stateless");
static_assert(std::is_empty<cms::log::OverwriteOldestOnFull>::value,
    "OverwriteOldestOnFull must remain stateless");
