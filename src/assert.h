
#pragma once

#define M_ASSERT(x, ...)                       \
    {                                          \
        if (!(x)) {                            \
            std::cout << "Assertion failed: "; \
            std::cout << __VA_ARGS__;          \
            std::cout << std::endl;            \
            assert(x);                         \
        }                                      \
    }
