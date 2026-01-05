
#pragma once

#include "assert.h"

[[noreturn]] inline void unreachable() {
    // Make this fail loudly in debug builds.
    invariant(false);
    // Uses compiler specific extensions if possible.
    // Even if no extension is used, undefined behavior is still raised by
    // an empty function body and the noreturn attribute.
#if defined(_MSC_VER) && !defined(__clang__)  // MSVC
    __assume(false);
#else  // GCC, Clang
    __builtin_unreachable();
#endif
}
