#include <cms/util/string_view.h>

constexpr cms::util::StringView standaloneView("view");
constexpr char paddedArray[8] = "abc";
constexpr char rawArray[3] = {'a', 'b', 'c'};
constexpr char embeddedArray[3] = {'A', '\0', 'B'};
constexpr cms::util::StringView paddedView(paddedArray);
constexpr cms::util::StringView rawView(rawArray);
constexpr cms::util::StringView embeddedView(embeddedArray);

static_assert(standaloneView.size() == 4, "string_view.h must compile independently");
static_assert(standaloneView[0] == 'v', "StringView literal construction must be constexpr");
static_assert(paddedView.size() == 3, "Array construction must stop at NUL");
static_assert(rawView.size() == 3, "Array construction must be bounded by N");
static_assert(embeddedView.size() == 1, "Embedded NUL must end array construction");
