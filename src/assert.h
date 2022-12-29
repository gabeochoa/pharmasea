
#pragma once

#include "std_include.h"

#define M_ASSERT(x, ...)                       \
    {                                          \
        if (!(x)) {                            \
            std::cout << "Assertion failed: "; \
            std::cout << __VA_ARGS__;          \
            std::cout << std::endl;            \
            assert(x);                         \
        }                                      \
    }

#define M_TEST_IMPL(x, y, op, op_string, ...)                          \
    {                                                                  \
        if (!(x op y)) {                                               \
            std::cout << "Test Failed\n";                              \
            std::cout << __PRETTY_FUNCTION__;                          \
            std::cout << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::cout << __VA_ARGS__ << "\n";                          \
            std::cout << x << op_string << y << "\n";                  \
            std::cout << "------ \n " << std::endl;                    \
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
