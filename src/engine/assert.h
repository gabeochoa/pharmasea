
#pragma once

#include <stdio.h>

#include <cassert>

#define VALIDATE(x, ...)                       \
    {                                          \
        if (!(x)) {                            \
            std::cout << "Assertion failed: "; \
            std::cout << __VA_ARGS__;          \
            std::cout << "\n";                 \
            assert(x);                         \
        }                                      \
    }

#define M_ASSERT(x, ...) VALIDATE(x, __VA__ARGS__)

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
    M_TEST_IMPL(x, y, <=, " was not greater than ", __VA_ARGS__)

#define M_TEST_NEQ(x, y, ...) \
    M_TEST_IMPL(x, y, !=, " was equal to ", __VA_ARGS__)
