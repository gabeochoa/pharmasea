#pragma once

#include "world_snapshot_v2.h"

#include "entity_helper.h"

namespace snapshot_v2 {

// Capture world state from a concrete entity list (typically EntityHelper::get_entities()).
[[nodiscard]] WorldSnapshotV2 capture_from_entities(const Entities& entities);

// Apply a snapshot by replacing the given entity list (and rebuilding ComponentStore-backed components).
// NOTE: This is a "full replace" apply used for initial MapInfo V2 wiring.
void apply_to_entities(Entities& entities, const WorldSnapshotV2& snap);

}  // namespace snapshot_v2

