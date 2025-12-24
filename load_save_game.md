# Load Save Game System Plan (with Diagetic “PS2 Save Select” Room + Replay Validation)
Date: 2025-12-24

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
  - day count (if available)
  - any progression summary you want visible

**Payload**
- Option A (recommended): `Map` (includes `LevelInfo`, hence entities, seed, was_generated)
- Option B: `LevelInfo` only (if you want saves independent of UI flags)

## Authoritative Load: Apply Save → World Reconstruction

### Non-negotiables
- Load must run **server-side** (authoritative) so the world is consistent for multiplayer and for replay.
- Loading must ensure `LevelInfo.was_generated = true` so the generator doesn’t wipe the loaded world.
- After installing entities, we must invalidate caches (named entity cache, walkable/path caches).

### “Apply Save” algorithm (server thread)
1. Read save header and payload from disk.
2. Deserialize payload into `Map`/`LevelInfo` using the same Bitsery polymorphic setup used by networking (`MyPolymorphicClasses` registration).
3. Install loaded state:
   - Replace server’s entity list with the loaded `entities`.
   - Ensure `was_generated = true`.
   - Ensure `seed` and `hashed_seed` match and are set.
4. Invalidate caches:
   - `EntityHelper::invalidateCaches()` and any other caches required (path/walkable).
5. Sync to clients:
   - Reuse existing map broadcast path (`Server::send_map_state()` already snapshots via `grab_things()` and sends the map).
6. Move players if needed:
   - Teleport players to appropriate spawn for the restored state (planning spawn marker, lobby origin, etc.).

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
  - A grid of N save slot pedestals (e.g., 8 or 12 like a PS2 card screen)
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
- Return players to a sensible location/state (or stay in room but now representing loaded world, depending on UX).

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

## Acceptance Criteria
- Loading a save results in a world that persists (no regen wipe) and matches the file.
- The diagetic Load Save room lists saves, shows metadata, and can load a slot reliably.
- `--load-save X --replay Y --replay-validate` completes and fails loudly on mismatches.
- No measurable impact to frame time during normal play (validation only when enabled).

