#pragma once

#include <cstdlib>
#include <cstdio>

namespace cms {
namespace test {

inline int& failureCount() noexcept {
    static int count = 0;
    return count;
}

inline int& checkCount() noexcept {
    static int count = 0;
    return count;
}

inline void check(
    bool condition,
    const char* expression,
    const char* file,
    int line) noexcept {
    ++checkCount();

    if (condition) {
        return;
    }

    ++failureCount();
    std::fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
}

inline void require(
    bool condition,
    const char* expression,
    const char* file,
    int line) noexcept {
    ++checkCount();

    if (condition) {
        return;
    }

    ++failureCount();
    std::fprintf(stderr, "%s:%d: requirement failed: %s\n", file, line, expression);
    std::exit(EXIT_FAILURE);
}

inline int finish() noexcept {
    const int checks = checkCount();
    const int failures = failureCount();

    if (checks == 0) {
        std::fprintf(stderr, "No checks were executed.\n");
        return 1;
    }

    if (failures == 0) {
        std::fprintf(stdout, "All %d check(s) passed.\n", checks);
        return 0;
    }

    std::fprintf(
        stderr,
        "%d of %d check(s) failed.\n",
        failures,
        checks);
    return 1;
}

} // namespace test
} // namespace cms

#define CMS_TEST_CHECK(expression) \
    ::cms::test::check(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

#define CMS_TEST_REQUIRE(expression) \
    ::cms::test::require(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
