#include <cms/status.h>

constexpr cms::WriteResult headerWriteResult{
    cms::Status::ok,
    1,
    1
};

constexpr cms::ParseResult<int> headerParseResult{
    cms::Status::ok,
    7,
    1
};

static_assert(headerWriteResult.written == 1, "status.h must compile independently");
static_assert(headerParseResult.value == 7, "status.h must expose ParseResult");
