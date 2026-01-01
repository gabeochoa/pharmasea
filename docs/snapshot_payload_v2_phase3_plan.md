# Pointer-free snapshot payload (V2) — Phase 3 plan

## Goal
Move save-game and network `ClientPacket::MapInfo` off legacy pointer-graph serialization (`Map -> LevelInfo -> vector<shared_ptr<Entity>> + PointerLinkingContext`) and onto a **pointer-free** snapshot payload (`snapshot_v2::WorldSnapshotV2`) that is safe for:

- **Save-game**: single-file persistence, no pointer linking, no polymorphic component pointers
- **Network map transfer**: chunked transport under SteamNetworkingSockets 512KB cap, no pointer linking, no per-entity polymorphic registration

Non-goal (for this phase): replacing all ongoing “per-tick” gameplay replication; this plan focuses on the **map/world snapshot transfer** path and save-game payload.

## Current reality (seam we will replace)
- `ClientPacket::MapInfo` currently contains `Map map;` and serializes it with `s.object(info.map);`
- `Map` serializes `LevelInfo game_info`
- `LevelInfo::serialize` uses `StdSmartPtr` over `entities` and relies on `PointerLinkingContext`
- `Entity` value serialization is currently provided by a TU-only shim (`src/entity_component_serialization.h`), which encodes presence bits + component payloads (no component pointers), but **the entity list is still a pointer-graph**.

## Design decisions (to avoid past failures)
### Identity and ID-collision avoidance (in-process host + client)
**Update (Afterhours Collections/Registry):** Afterhours now supports multiple independent ECS “worlds” via `afterhours::EntityCollection` + `afterhours::ScopedEntityCollection` (TLS-bound). This is the preferred way to avoid host+client collisions in one process.

**Rule:** never apply a server snapshot by creating/overwriting entities using server-side `legacy_id`.

- With **Collections** correctly bound, server and client can safely reuse the same numeric `EntityID` values because each collection owns its own `ComponentStore` and `EntityHelper`.
- You still must not treat `legacy_id` as stable identity across runs; it’s debug-only.
- **When applying a snapshot across collections** (server → client world), you may still need a remap table **if any component payload contains intra-world references** (e.g. `EntityID` “parent/held item” fields). That remap is about *relationship pointers*, not about preventing component-store collisions.
  - `server EntityHandle` → `client EntityHandle` (preferred) or → `client EntityID`

**Outcome:** server and client can coexist in one process without clobbering component state, *as long as each thread is bound to the correct `EntityCollection`* when touching Afterhours ECS APIs.

### Afterhours Collections integration (required for Phase 3)
PharmaSea must explicitly bind the active Afterhours collection on each thread that accesses ECS:

- Create two collections:
  - `afterhours::EntityCollection server_collection;`
  - `afterhours::EntityCollection client_collection;`
- In the server thread, wrap ECS work:
  - `afterhours::ScopedEntityCollection scoped(server_collection);`
- In the client thread, wrap ECS work:
  - `afterhours::ScopedEntityCollection scoped(client_collection);`

Important notes:
- `afterhours::ComponentStore::get()` and `afterhours::EntityHelper::get()` now assert if no collection is set (no global fallback).
- Entity creation should go through the collection-bound path (e.g. `afterhours::EntityHelper::createEntity*` while scoped), not `std::make_shared<afterhours::Entity>()`, otherwise you’ll bypass per-collection ID generation and can reintroduce collisions/confusion.

### One true entity-id generator
Eliminate the “one counter per translation unit” issue:

- Afterhours has fixed its own generator to be a single definition (`inline static std::atomic_int ENTITY_ID_GEN{0};`) and also introduced per-collection ID generation (`EntityCollection::entity_id_gen`).
- PharmaSea should still avoid defining TU-local `static std::atomic_int` generators in headers for entity IDs or other identity systems.

**Acceptance:** IDs are globally unique across the binary; no duplicate component spam caused by collisions.

### Components: DTO projection over copying BaseComponent
To avoid copy/move incompatibilities (non-copyable `BaseComponent` and pooled storage relocation issues):

- Snapshot payload contains **DTOs** (like `TransformV2`) rather than copying the live component type when copy/move is problematic.
- Capture: `live_component -> dto`
- Apply: `dto -> live_component` (via `addComponent<T>()` + field assignment)

### Network: chunking from day 1
**Rule:** never send a single “full snapshot” message.

- Introduce a `MapSnapshotV2` network protocol that sends:
  - **Begin**: metadata (snapshot version, total compressed bytes, chunk size, chunk count, checksum/hash)
  - **Chunk[N]**: payload bytes split into <= ~480KB safe chunks
  - **End/Ack**: client confirms receipt; server may resend missing chunks (optional for phase 3 if using reliable ordered channel and low loss)

Notes:
- Keep each chunk under the hard cap (`524288`). Use a conservative ceiling (e.g. 480KB) to account for headers/overhead.
- The payload itself can be a bitsery-serialized `WorldSnapshotV2` byte buffer (optionally compressed later).

### Performance: avoid per-entity polymorphic context rebuild
V2 snapshot is intentionally pointer-free and should not require polymorphic registration per entity.

- Build serializer/deserializer context **once per snapshot** (or even reuse across calls), not per entity.
- Capture/apply should be “flat”: one pass over entities for `EntityRecordV2`, and per-component passes for component lists.

## Implementation plan (phased, keep the game running throughout)
### Phase 3A — Add capture/apply API for `WorldSnapshotV2` (no wiring yet)
Create new module(s), e.g.:
- `src/world_snapshot_v2_capture.h/.cpp`
- `src/world_snapshot_v2_apply.h/.cpp`

Functions (shape, not final names):
- `snapshot_v2::WorldSnapshotV2 capture_world_v2(const Map& map_or_levelinfo)`
- `void apply_world_v2(Map& map_or_levelinfo, const snapshot_v2::WorldSnapshotV2& snap, ApplyOptions opts)`
  - `ApplyOptions` includes:
    - `afterhours::EntityCollection& target_collection` (client or server)
    - `bool remap_entity_refs = true` (default true if any components still store `EntityID` references)
    - **No longer required for collision avoidance:** `remap_entity_ids` (Collections isolate the stores), but remapping may still be needed for relationship fields.

Initial component coverage:
- `EntityRecordV2` fields (handle, entity_type, tags, cleanup, component_set)
- `TransformV2` list as the first wired component (already defined in `src/world_snapshot_v2.h`)

Acceptance:
- Can round-trip capture→apply within a single world instance (offline) without crashes.

### Phase 3B — Replace save-game world payload with V2 snapshot (keep header)
Update `SaveGameFile` to store V2 snapshot instead of `Map`:

- `SaveGameHeader` stays and continues to serialize first (fast header read).
- Replace `Map map_snapshot;` with something like:
  - `snapshot_v2::WorldSnapshotV2 world_snapshot_v2;`
  - plus any minimal extra fields that are *not* part of the world snapshot (e.g., minimap flag, seed) if needed.

Migration strategy:
- Support loading legacy saves for a transition period:
  - If `save_version == 1`, load old `Map` and immediately re-save as V2 on next save (or convert in-memory).
  - If `save_version == 2`, load `WorldSnapshotV2`.

Acceptance:
- `make -f makefile_cursor cursor-check` passes.
- Existing “complete-day-test” still passes.
- Saving and loading a slot works and results in the same gameplay state (at least for entities/components covered in V2; during rollout we may temporarily keep fallback paths for uncovered components).

### Phase 3C — Replace `ClientPacket::MapInfo` with V2 snapshot transport
Protocol change:
- Replace `ClientPacket::MapInfo { Map map; }` with a V2 transfer flow:
  - either `ClientPacket::MapSnapshotBegin/Chunk/End` messages, or
  - a single `MapInfoV2` variant that contains **one chunk** (and a session id + indices) and is sent multiple times.

Client behavior:
- Buffer chunks until complete, then deserialize payload bytes into `WorldSnapshotV2`.
- Apply snapshot while bound to the **client** `EntityCollection` (`ScopedEntityCollection`).
- Enable entity-reference remapping if any payload still uses `EntityID` relationships.
- Only swap the “current map” once fully applied to avoid half-applied state.

Server behavior:
- Only send map snapshot on join / map change (not per frame).

Acceptance:
- No disconnects from oversize messages.
- No duplicate component spam / no crashes in in-process host+client.

### Phase 3D — Retire pointer-linking requirements on map transfer and saves
Once save-game + network map transfer are V2:
- Remove `PointerLinkingContext` from those serialization contexts where no longer needed.
- Stop including `StdSmartPtr` serialization for the map snapshot path.
- Keep the `Entity` shim only as long as some other legacy path still needs it; otherwise plan its removal.

Acceptance:
- Guardrails still pass:
  - `scripts/check_pointer_free_serialization.py`
  - `scripts/check_no_persistent_refentity.py`
  - `scripts/check_network_polymorphs.py`

## Risk checklist (explicitly addressing prior issues)
- **In-process collisions**: bind server/client to separate `afterhours::EntityCollection` instances; never rely on shared global stores. Do not reuse `legacy_id` as identity.
- **ENTITY_ID_GEN per TU**: Afterhours fixed its generator; avoid introducing new TU-local generators in PharmaSea.
- **BaseComponent copy/move**: snapshot DTOs, not live component copies.
- **Bitsery container4b asserts (macOS/libc++)**: do not serialize raw byte buffers with `container4b` / `text4b`; use `container1b`/`text1b` for bytes/strings. (For V2 transfer, treat the snapshot payload as bytes and serialize it with a 1-byte element container.)
- **512KB message cap**: chunking protocol required; conservative max chunk size.
- **Server perf regression**: no per-entity polymorphic context rebuild; snapshot only on join/map change; avoid polymorphs entirely in V2.
- **Shutdown crash after network failure**: avoid oversize disconnect path; keep teardown unaffected.

## Definition of done (Phase 3)
- Save-game stores and loads `WorldSnapshotV2` (with legacy load fallback as needed).
- Network map transfer uses chunked V2 snapshot payload and stays under message size cap.
- No pointer linking / smart pointer graph is required for save or map transfer.
- Game still builds and passes the existing “complete-day-test” scenario.

