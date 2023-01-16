
#pragma once

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wshadow"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#ifdef ENABLE_TRACING

#define TRACY_ENABLE 1

#include <tracy/tracy/Tracy.hpp>
#define TRACY_FRAME_MARK(x) FrameMark
#define TRACY_ZONE_SCOPED ZoneScoped
#define TRACY_ZONE(x) ZoneNamedN(x, #x, true)
#define TRACY_ZONE_NAMED(x, y, z) ZoneNamedN(x, y, z)
#define TRACY_LOG(msg, size) TracyMessage(msg, size)

#else

#define TRACY_FRAME_MARK(x) void
#define TRACY_ZONE_SCOPED void
#define TRACY_ZONE_NAMED(x, y, z) void
#define TRACY_LOG(msg, size) void

#endif

/// /

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif
