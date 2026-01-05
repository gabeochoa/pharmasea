
#pragma once

#include <cassert>
#include <iostream>

// Invariants:
// - Enabled in debug builds by default.
// - Compiled out when `NDEBUG` is defined (typical "production" builds).
// - You can force-enable invariants in release by defining PHARMASEA_FORCE_INVARIANTS.
#if !defined(NDEBUG) || defined(PHARMASEA_FORCE_INVARIANTS)
#define PHARMASEA_INVARIANTS_ENABLED 1
#else
#define PHARMASEA_INVARIANTS_ENABLED 0
#endif

#if PHARMASEA_INVARIANTS_ENABLED
#define invariant(condition)                                                     \
    do {                                                                         \
        const bool _pharmasea_ok = static_cast<bool>(condition);                 \
        if (!_pharmasea_ok) {                                                    \
            std::cerr << "Invariant failed: " << #condition << " (" << __FILE__  \
                      << ":" << __LINE__ << ")\n";                               \
            assert(false);                                                       \
        }                                                                        \
    } while (0)
#else
// NOTE: don't evaluate `condition` in production builds.
#define invariant(condition)                                                     \
    do {                                                                         \
        (void) sizeof(condition);                                                \
    } while (0)
#endif

// VALIDATE:
// - Always runs (even in production builds).
// - Prints a helpful message if it fails.
// - Triggers an assert in debug builds.
#define VALIDATE(condition, msg_expr)                                            \
    do {                                                                         \
        const bool _pharmasea_ok = static_cast<bool>(condition);                 \
        if (!_pharmasea_ok) {                                                    \
            std::cerr << "Validation failed: " << #condition << " (" << __FILE__ \
                      << ":" << __LINE__ << "): " << (msg_expr) << "\n";         \
            if (PHARMASEA_INVARIANTS_ENABLED) {                                   \
                assert(false);                                                   \
            }                                                                    \
        }                                                                        \
    } while (0)

#define M_ASSERT(condition, msg_expr)                                            \
    do {                                                                         \
        /* Keep M_ASSERT as debug-only, message is ignored intentionally. */      \
        invariant(condition);                                                    \
        (void) sizeof(msg_expr);                                                  \
    } while (0)

#define M_TEST_IMPL(x, y, op, op_string, ...)                          \
    {                                                                  \
        if (!(x op y)) {                                               \
            std::cout << "Test Failed\n";                              \
            std::cout << __PRETTY_FUNCTION__;                          \
            std::cout << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::cout << __VA_ARGS__ << "\n";                          \
            std::cout << x << op_string << y << "\n";                  \
            std::cout << "------ \n ";                                 \
            assert(x op y);                                            \
        }                                                              \
    }

#define M_TEST_EQ(x, y, ...) \
    M_TEST_IMPL(x, y, ==, " did not equal ", __VA_ARGS__)

#define M_TEST_LEQ(x, y, ...) \
    M_TEST_IMPL(x, y, <=, " was not less than ", __VA_ARGS__)

#define M_TEST_GEQ(x, y, ...) \
    M_TEST_IMPL(x, y, >=, " was not greater than ", __VA_ARGS__)

#define M_TEST_NEQ(x, y, ...) \
    M_TEST_IMPL(x, y, !=, " was equal to ", __VA_ARGS__)

#define M_TEST_T(x, ...) M_TEST_IMPL(x, true, ==, " was not true ", __VA_ARGS__)

#define M_TEST_F(x, ...) \
    M_TEST_IMPL(x, false, ==, " was not false", __VA_ARGS__)
