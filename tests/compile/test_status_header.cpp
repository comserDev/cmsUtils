#include <cms/util/status.h>

constexpr cms::util::WriteResult headerWriteResult{
    cms::util::Status::ok,
    1,
    1
};

constexpr cms::util::ParseResult<int> headerParseResult{
    cms::util::Status::ok,
    7,
    1
};

static_assert(headerWriteResult.written == 1, "status.h must compile independently");
static_assert(headerParseResult.value == 7, "status.h must expose ParseResult");
