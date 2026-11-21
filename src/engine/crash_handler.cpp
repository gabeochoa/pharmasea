#if !defined(NDEBUG)
#ifdef _WIN32
// backward.hpp includes <psapi.h> before <windows.h>; psapi needs it first.
#include <windows.h>
#endif
#include "backward/backward.hpp"

namespace backward {
SignalHandling sh;
}
#endif