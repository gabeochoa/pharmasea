//
//          Tracy profiler
//         ----------------
//
// For fast integration, compile and
// link with this source file (and none
// other) in your executable (or in the
// main DLL / shared object on multi-DLL
// projects).
//

// Define TRACY_ENABLE to enable profiler.

// PHARMASEA ENABLE
#define TRACY_ENABLE

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wshadow"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include "common/TracySystem.cpp"

#ifdef TRACY_ENABLE

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include "client/TracyAlloc.cpp"
#include "client/TracyCallstack.cpp"
#include "client/TracyDxt1.cpp"
#include "client/TracyOverride.cpp"
#include "client/TracyProfiler.cpp"
#include "client/TracySysTime.cpp"
#include "client/TracySysTrace.cpp"
#include "client/tracy_rpmalloc.cpp"
#include "common/TracySocket.cpp"
#include "common/tracy_lz4.cpp"

#if TRACY_HAS_CALLSTACK == 2 || TRACY_HAS_CALLSTACK == 3 || \
    TRACY_HAS_CALLSTACK == 4 || TRACY_HAS_CALLSTACK == 6
#include "libbacktrace/alloc.cpp"
#include "libbacktrace/dwarf.cpp"
#include "libbacktrace/fileline.cpp"
#include "libbacktrace/mmapio.cpp"
#include "libbacktrace/posix.cpp"
#include "libbacktrace/sort.cpp"
#include "libbacktrace/state.cpp"
#if TRACY_HAS_CALLSTACK == 4
#include "libbacktrace/macho.cpp"
#else
#include "libbacktrace/elf.cpp"
#endif
#include "common/TracyStackFrames.cpp"
#endif

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma warning(pop)
#endif

#endif

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif
