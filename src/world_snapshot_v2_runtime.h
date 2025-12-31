#pragma once

#include "world_snapshot_v2.h"

#include "entity_helper.h"

namespace snapshot_v2 {

struct ApplyOptionsV2 {
    // If true, overwrite the newly-created entity IDs with snapshot legacy_id.
    // This preserves legacy EntityID-based references but is unsafe when
    // running an in-process host+client (shared ComponentStore).
    bool preserve_legacy_entity_ids = true;

    // If true, remove pooled components for the entities being replaced before
    // applying the new snapshot.
    //
    // IMPORTANT: When hosting in-process (server + client in one process),
    // ComponentStore is global and keyed by EntityID, and the server uses the
    // same EntityIDs. Removing components here would clobber the server world.
    bool clear_existing_components = true;
};

// Capture world state from a concrete entity list (typically EntityHelper::get_entities()).
[[nodiscard]] WorldSnapshotV2 capture_from_entities(const Entities& entities);

// Apply a snapshot by replacing the given entity list (and rebuilding ComponentStore-backed components).
// NOTE: This is a "full replace" apply used for initial MapInfo V2 wiring.
void apply_to_entities(Entities& entities, const WorldSnapshotV2& snap,
                       ApplyOptionsV2 options = {});

}  // namespace snapshot_v2

