#pragma once

// DEPRECATED: This header is kept for backward compatibility
// New code should use network/types.h for types and network/serialization.h for
// serialization This header forwards to the new structure

#include "types.h"

// For backward compatibility, also include serialization types if needed
// But note: serialization.h includes heavy headers, so only include this
// in files that actually need serialization
#ifdef NETWORK_NEED_SERIALIZATION
#include "serialization.h"
#endif

namespace network {
// All types are now in types.h
// Serialization functions are in serialization.cpp
// Serialize template is in serialization.h
}  // namespace network
