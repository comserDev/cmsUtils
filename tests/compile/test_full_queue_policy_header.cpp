#include <type_traits>

#include <cms/util/log/full_queue_policy.h>

static_assert(std::is_empty<cms::util::log::RejectOnFull>::value,
    "RejectOnFull must remain stateless");
static_assert(std::is_empty<cms::util::log::OverwriteOldestOnFull>::value,
    "OverwriteOldestOnFull must remain stateless");
