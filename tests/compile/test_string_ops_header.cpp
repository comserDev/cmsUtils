#include <cms/string_ops.h>

static_assert(
    cms::string::npos == static_cast<std::size_t>(-1),
    "string_ops.h must compile independently");
