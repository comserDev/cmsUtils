#include <cms/util/format.h>
#include <cms/util/log/async_logger.h>
#include <cms/util/parse.h>
#include <cms/util/static_queue.h>
#include <cms/util/static_string.h>
#include <cms/util/sync/synchronized_queue.h>
#include <cms/util/utf8.h>

namespace cms {

struct Status {};
struct Controller {};

} // namespace cms

static_assert(sizeof(cms::Status) > 0,
    "V2 public headers must not occupy cms::Status");
static_assert(sizeof(cms::Controller) > 0,
    "V2 public headers must allow application-owned cms names");
