# Pointer-free snapshot payload (V2) — Phase 3 plan (updated for Afterhours Collections)

## Goal
Replace legacy pointer-graph serialization for **save-game** and **network MapInfo** with a single pointer-free payload (`snapshot_v2::WorldSnapshotV2`), now that Afterhours supports multiple independent ECS worlds via `afterhours::EntityCollection`.

Scope for Phase 3:
- **Save-game**: no pointer linking, no polymorphic component pointers.
- **Network map transfer (MapInfo)**: no pointer linking, respects SteamNetworkingSockets constraints (512KB max message) via chunking.

Non-goal:
- Replacing ongoing per-tick gameplay replication; this plan is about the **map/world snapshot transfer** boundary and persistence.

## Legacy seam (what we replace)
- Network: `ClientPacket::MapInfo` serializes a full `Map` (`s.object(info.map)`).
- `Map` serializes `LevelInfo`.
- `LevelInfo::serialize` uses `StdSmartPtr` over entities and requires `PointerLinkingContext`.
- Component values are currently serialized via `src/entity_component_serialization.h` (value-only shim), but the **entity list is still pointer-graph based**.

## Afterhours Collections: new baseline assumption
Afterhours now provides:
- `afterhours::EntityCollection`: owns `ComponentStore`, `EntityHelper`, and an entity id generator.
- `afterhours::ScopedEntityCollection`: binds the active collection per-thread (TLS).
- `ComponentStore::get()` / `EntityHelper::get()` are collection-bound and assert if no collection is active.

### PharmaSea requirements (prerequisite for Phase 3)
**Rule:** any code that touches Afterhours ECS APIs must run with the correct collection bound on that thread.

Implementation shape:
- Maintain two collections:
  - `server_collection` (server/host world)
  - `client_collection` (client world)
- In each thread’s main loop, establish a long-lived scope:
  - `afterhours::ScopedEntityCollection scoped(server_collection);`
  - `afterhours::ScopedEntityCollection scoped(client_collection);`

Entity creation rule:
- Create entities through collection-bound APIs while scoped (e.g. `afterhours::EntityHelper::createEntity*`), not via `std::make_shared<afterhours::Entity>()` or default `Entity()`.

Impact:
- The old “ComponentStore is global keyed by EntityID” collision is solved by construction: server and client worlds have separate stores.

## Snapshot V2: identity and apply rules
### Identity in the payload
- Payload keys entities by `afterhours::EntityHandle` (slot + generation) plus debug `legacy_id`.
- Handles are **not portable across collections**.

### Apply across collections (server → client)
When applying a snapshot into a different collection:
- You cannot resolve snapshot handles in the target collection.
- Treat the handle as a stable key *within the payload* and build a mapping:
  - `payload_handle` → `new_target_entity`
- Apply component list entries to the mapped entity.
- If any DTOs include relationships (stored as `EntityID`/handles), remap those relationships using the same mapping.

## Network reality (hard constraint)
SteamNetworkingSockets disconnects on messages > 512KB. Full snapshots can be MBs.

**Rule:** MapInfo transfer must be chunked (Begin/Chunk/End or equivalent).

## Phased plan (minimal changes, stable throughout)
### Phase 3.0 — Adopt Collections in PharmaSea
Goal: isolate server/client ECS state correctly.

Acceptance:
- No Afterhours asserts about missing active collection.
- No duplicate-component spam/crashes caused by shared component storage.

### Phase 3.1 — Implement `WorldSnapshotV2` capture/apply glue
Add new code (names flexible):
- `src/world_snapshot_v2_capture.h/.cpp`
- `src/world_snapshot_v2_apply.h/.cpp`

API shape:
- `snapshot_v2::WorldSnapshotV2 capture_world_v2(const LevelInfo& level)`
- `void apply_world_v2(LevelInfo& level, const snapshot_v2::WorldSnapshotV2& snap, ApplyOptions opts)`

`ApplyOptions`:
- `afterhours::EntityCollection& target_collection`
- `bool remap_entity_relationships = true` (only if DTOs contain relationships)

Initial coverage:
- entity metadata (`EntityRecordV2`)
- `TransformV2` list

Acceptance:
- Capture/apply works within one collection.
- Capture on server collection → apply to client collection works without touching server state.

### Phase 3.2 — Save-game uses V2 payload
Change save format:
- Keep `SaveGameHeader` first for fast reads.
- Replace `SaveGameFile.map_snapshot` with `snapshot_v2::WorldSnapshotV2` (or serialized bytes of it).
- Bump save version to 2.
- Keep legacy load (v1) temporarily if needed, converting to v2 in-memory.

Acceptance:
- Builds and runs (including complete-day-test).

### Phase 3.3 — Network MapInfo uses V2 chunked transfer
Protocol:
- Replace `ClientPacket::MapInfo{ Map map; }` with Begin/Chunk/End messages carrying serialized `WorldSnapshotV2` bytes.

Client:
- Buffer chunks, deserialize `WorldSnapshotV2`, apply under `ScopedEntityCollection(client_collection)`.

Server:
- Serialize snapshot once per join/map-change and stream chunks reliably.

Acceptance:
- No oversize-message disconnects.
- Map transfer no longer requires pointer linking.

### Phase 3.4 — Remove legacy pointer-linking usage from these paths
Once save + MapInfo are V2:
- Remove `PointerLinkingContext` from those serialization contexts where unused.
- Stop relying on `StdSmartPtr` for map transfer/persistence.
- Remove the entity-component serialization shim if no longer needed elsewhere.

Acceptance:
- Guardrails still pass:
  - `scripts/check_pointer_free_serialization.py`
  - `scripts/check_no_persistent_refentity.py`
  - `scripts/check_network_polymorphs.py`

## Risk checklist (updated)
- **Collections not bound**: any ECS access without `ScopedEntityCollection` will assert.
- **Accidental default entity creation**: bypasses per-collection id generation and can reintroduce confusing behavior.
- **Relationship fields**: if DTOs serialize `EntityID` relationships, they must be remapped on apply.
- **512KB cap**: MapInfo must be chunked.
- **Bitsery byte containers**: use 1-byte element containers for raw payload bytes.

