
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

// TODO add a note for whether we need these voids
#define TRACY_FRAME_MARK(x) 0
#define TRACY_ZONE_SCOPED 0
#define TRACY_ZONE(x) 0
#define TRACY_ZONE_NAMED(x, y, z) 0
#define TRACY_LOG(msg, size) 0

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
