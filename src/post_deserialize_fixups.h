#pragma once

#include "entity_helper.h"

// When deserializing from a save file or over the network, some components keep
// runtime-only state (callbacks) or self/relationship handles that should be
// re-established after the full entity list is available.
//
// Keep this limited to "safe" fixups that only use existing component APIs.
namespace post_deserialize_fixups {
void run(Entities& entities);
}

