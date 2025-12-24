# Load Save Game System Plan (with Diagetic “PS2 Save Select” Room + Replay Validation)
Date: 2025-12-24

## Decisions (confirmed)
- **World persistence**: **Hybrid** (persist player-altered state; regenerate baseline from seed).
- **Save timing**: **Only allow saving in Planning mode** (daytime).
- **Load destination**: After load, land in **Planning mode**.
- **Slots/UI**: Show a **PS2-style grid** (8–12 slots) in a diagetic Load Save room.
- **Versioning**: **Backwards-compatible additions** (versioned reads).
- **Preview metadata**: Include **seed/day/playtime/coins/unlocks summary**; **thumbnail preview is a stretch goal**.
- **Replay validation**: **End-state assertions only** (no per-frame checks); replay remains **CLI-driven** for now.
- **Multiplayer**: **Host authoritative** load (clients sync + teleport).
- **Room features**: **Load + Delete** in-room for v1; **Replay UI later** (CLI flag now).
-
- **Hybrid scope v1**: **Furniture placements only** (items do not persist; they respawn from furniture).
- **Future scope**: likely expands toward “C” (broader saveable set), but start narrow.
- **Saving UX**: both **diagetic Save Station** + **menu button** (enabled only in Planning).
- **Delete UX**: **hold-to-delete** (long press) for now.
- **Thumbnail previews**: prefer **minimap-style preview** (similar in spirit to seed UI), with a **screenshot fallback** if minimap render is non-trivial.

## Goals
- **Load an existing save file** and reconstruct the **authoritative world/map** to match it.
- Ensure loading does **not** get overwritten by procedural generation (`ensure_generated_map`).
- Add a **diagetic UI** (in-world room) to browse save slots “PS2-style” and load one.
- Add a **replay mode** that can start from a save, run deterministic input replay, and **validate** the world state.

## Current Code Reality (what exists today)

### Serialization foundations already exist
- `Entity` serializes ids, tags, and polymorphic components (`src/entity.h`).
- `LevelInfo` serializes `entities`, `seed`, `was_generated`, `hashed_seed` (`src/level_info.h`).
- `Map` serializes `LevelInfo` (`src/map.h`).
- Networking already serializes `Map` inside `ClientPacket::MapInfo` (`src/network/serialization.h`).

### The missing piece: “apply loaded map to runtime”
- `Map::update_map()` currently only copies `showMinimap` (`src/map.cpp`).
- `Map::_onUpdate()` always calls `game_info.ensure_generated_map(seed)` (`src/map.cpp`).
- `LevelInfo::ensure_generated_map()` deletes entities + regenerates unless `was_generated == true` (`src/level_info.cpp`).

Conclusion: even though `Map`/`LevelInfo` can be serialized, the codebase lacks a robust “install this world state” path.

### Replay/validation already exists
- Replays load from `recorded_inputs/*.txt` and inject input (`src/engine/simulated_input/input_replay.cpp`).
- Validation hooks exist (`replay_validation::*`) and can fail hard at end (`src/engine/simulated_input/replay_validation.*`).
- One example validation spec exists: `completed_first_day` checks `HasDayNightTimer` (`src/tests/replay_specs.cpp`).

## Authority Model (important for intern)
This project has a clear split:
- **Server thread** is the **authoritative simulation** (it runs `SystemManager::update_all_entities()`; client thread early-returns).
- **Client thread** is primarily **input/UI/render** and should be driven by server sync snapshots.

**Save/Load rule (v1):**
- **Only the host may Save/Load**, and the action must run on the **host’s server thread**.
- Clients (including the host’s own client thread) should treat incoming world data as **render/sync state**, not as authoritative gameplay state.

**Implication for design:**
- We may choose to serialize/send only what’s needed to render (a “RenderSnapshot”) over the network.
- Save files, however, must be sourced from the authoritative server state so we can restore correctly (IDs, references, progression, etc.).

## Save File Format (what we’ll persist)

### SaveGame file structure (binary)
Use Bitsery, but add a small header for **versioning + UI metadata**.

**Header** (fixed-ish, quick to read without deserializing the world):
- magic: `"PHARMSAVE"` (or similar)
- save_version: integer
- build/version identifier: `HASHED_VERSION` (or `VERSION`)
- timestamp
- display_name (optional)
- preview fields for showroom:
  - seed (string)
  - day count (planning day index)
  - playtime seconds (or hh:mm)
  - coins/balance + cart (if relevant)
  - unlocks/progression summary (compact)
  - **optional**: thumbnail(s) (stretch)
    - `slot_XX_minimap.png` (preferred)
    - `slot_XX_screenshot.png` (fallback if easier)

**Payload (hybrid model)**
- Baseline: `seed` (used to regenerate the deterministic base map)
- Delta: “player-altered state”, e.g.
  - moved/placed **furniture** (v1 scope)
  - progression + round settings + bank + day/night timer state
  - any other state designers consider “save-worthy”

**Implementation note (practical)**
- Phase 1 can ship using a full snapshot payload (`Map` / `LevelInfo`) to prove correctness end-to-end.
- Phase 2 refactors to true hybrid delta (filter + stable identifiers), once load/apply is solid.

## Authoritative Load: Apply Save → World Reconstruction

### Non-negotiables
- Load must run **server-side** (authoritative) so the world is consistent for multiplayer and for replay.
- Loading must ensure `LevelInfo.was_generated = true` so the generator doesn’t wipe the loaded world.
- After installing entities, we must invalidate caches (named entity cache, walkable/path caches).
- **Host-only**: only host server thread can apply saves; clients are synced afterward.

### “Apply Save” algorithm (server thread)
1. Read save header and payload from disk.
2. Generate baseline world from `seed` (deterministic):
   - Ensure map generator runs once on server to create baseline entities.
3. Apply delta from save:
   - Create/update/delete entities that represent player-altered state.
   - Apply progression/settings/bank/day-night state to the authoritative entities (e.g., Sophie, timer entity).
4. Invalidate caches:
   - `EntityHelper::invalidateCaches()` and any other caches required (path/walkable).
5. Sync to clients:
   - Reuse existing map broadcast path (`Server::send_map_state()` already snapshots via `grab_things()` and sends the map).
6. Move players if needed:
   - Teleport players to **Planning spawn** (target outcome: load always lands in Planning mode).

### Saving (planning-only)
- Only allow save writes while in Planning mode (daytime).
- Attempting to save outside Planning should be a no-op with clear feedback (toast/log).
- UX sources:
  - Diagetic **Save Station** in Planning (recommended primary)
  - Menu button (only enabled in Planning) as a convenient shortcut

### Save source-of-truth (host-only)
- Save is captured from the **server-thread authoritative state**, then written to disk.
- After writing, clients do not need to “agree”; they will receive state via normal sync.

### Save locations & slots
Use a predictable folder under the game folder:
- `{SaveGamesFolder}/{GAME_FOLDER}/saves/slot_01.bin`, `slot_02.bin`, …
- Use atomic writes: write temp then rename.

## Diagetic UI: “Load Save” Room (PS2-style Save Select)

### Core idea
Create a new in-world “showroom” state like `ModelTest`, where each save slot is a physical pedestal/box.

Each slot pedestal entity shows:
- `HasName`: `"Slot 1 — Day 3 — seed XYZ — v0.24.09.06"` (etc.)
- A renderer component (simple colored box or model)
- An interaction behavior that triggers `load_save(slot)`

### Entry point: lobby trigger
In `LevelInfo::generate_lobby_map()`:
- Add a new `IsTriggerArea` similar to existing lobby triggers:
  - `IsTriggerArea::Lobby_LoadSave` (new)
  - Transition to new game state: `game::State::LoadSaveRoom` (new)

### Room generation
Add `LevelInfo::generate_load_save_room_map()` (pattern after `generate_model_test_map()`):
- Spawn:
  - “Back to Lobby” trigger
  - A grid of N save slot pedestals (**8–12**) like a PS2 card screen
  - Optional extra stations:
    - “Delete Save”
    - “Rename Save”
    - “Start Replay”

### Save discovery for the room (fast)
Don’t load full worlds just to show text:
- Enumerate files in saves directory
- Read only `SaveGameHeader` to populate pedestal labels and “compatibility” coloring

### Slot interaction behavior
When player interacts with pedestal:
- Server loads and applies the save (authoritative).
- Clients receive sync.
- Transition to **Planning** and place players at planning spawn.

### Delete behavior (v1)
- In the room, provide **hold-to-delete** (long press) on the slot pedestal.
- Delete removes the save file (and thumbnail file if present) and refreshes the room list.

Fast shipping version:
- Start with **3 fixed slots** (A/B/C) as pedestals.
- Upgrade to dynamic enumeration once stable.

## Replay: Load Save + Validate

### Desired CLI / dev flow
Support running:
- `--load-save <slot-or-path> --replay <name> --replay-validate`

Runtime sequence:
1. Server starts
2. Load+apply save
3. Replay injects inputs from `recorded_inputs/<name>.txt`
4. On replay end, validation spec runs and asserts expected world conditions

### Validation strategy
Validation should be **cheap** (no per-frame hashing):
- Validate at replay start and/or end.

Pick an expectation source:
- **A) Save-as-truth**: at replay start, assert that the loaded world matches the save snapshot (counts, critical entities, transforms).
- **B) Replay snapshot**: store a small expected hash alongside replay and compare at end.

Suggested high-signal checks:
- seed / hashed_seed
- existence of critical entities (e.g., Sophie, day/night timer entity, floor markers)
- positions/facing of key entities
- selected component values that matter for “settings in right spot”

Wire failures via `replay_validation::add_failure()` and fail in `end_replay()`.

## Implementation Phases (recommended)
### Phase 1 — Ship the pipeline (snapshot-based)
- Implement save header + slot enumeration + delete
- Implement “apply loaded world” using a full snapshot payload (fast correctness)
- Add Load Save room (8–12 slots) + lobby trigger + transitions
- Add planning-only save restriction
- Add CLI `--load-save` and replay end-state validation hooks

### Phase 2 — Convert to true hybrid delta
- Decide baseline vs player-altered classification (tags/component or entity-type whitelist)
- Add stable identifiers for delta entities (so we can match/update them across regen)
- Save delta + apply delta on top of regenerated baseline

### Phase 3 — Thumbnail previews (stretch)
- On save: capture previews (pick easiest first):
  - **Fast path**: simple screenshot → `slot_XX_screenshot.png`
  - **Preferred**: minimap-style render → `slot_XX_minimap.png`
- In room: load and display thumbnail on pedestal (keep it cheap: cache textures)

## Acceptance Criteria
- Loading a save results in a world that persists (no regen wipe) and matches the file.
- The diagetic Load Save room lists saves, shows metadata, and can load a slot reliably.
- `--load-save X --replay Y --replay-validate` completes and fails loudly on mismatches.
- No measurable impact to frame time during normal play (validation only when enabled).

