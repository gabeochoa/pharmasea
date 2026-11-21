# Load Save Game System Plan (with Diagetic “PS2 Save Select” Room + Replay Validation)
Date: 2025-12-24

## Status (as of 2025-12-24)

### Implemented (Phase 1: snapshot pipeline)
- **Save file format**: Bitsery-based `SaveGameHeader` + `SaveGameFile` (full snapshot payload `Map`) per slot, with `"PHARMSAVE"` magic and build/version fields.
- **Save location**: `Files::get().game_folder()/saves/slot_XX.bin` (atomic write via `*.tmp` + rename).
- **Slot discovery**: enumerate `kDefaultNumSlots` (currently **8**) and read **header only** to populate UI metadata.
- **Authoritative apply (server-only)**: load installs snapshot on the **server thread**, sets `was_generated = true`, invalidates caches, and forces a map sync to clients (`force_send_map_state()`).
- **Load destination**: after load, players are moved to `game::State::InGame` (planning spawn via `Planning_SpawnArea` floor marker).
- **Diagetic Load/Save room**:
  - Lobby trigger → `game::State::LoadSaveRoom`
  - Room generator spawns **Back to Lobby**, **Delete Mode toggle**, and **slot pedestals** (trigger areas with slot id in `HasSubtype`)
  - Slot labels are **multi-line**; slot label font is reduced for `LoadSave_LoadSlot` and `Planning_SaveSlot` trigger areas
  - **Empty slots** are **grey**
  - **Delete mode**: occupied slots render **red**; empty stays **grey**
- **Delete UX**: selecting a slot pedestal either **loads** or **deletes** depending on delete-mode toggle; deletion regenerates the room list.
- **Planning-only save**: a temporary Planning save trigger exists (currently hard-coded to slot 01), and attempts outside planning/daytime are rejected with an announcement (surfaced as a toast).
- **Toasts**: announcement packets are displayed as in-game toasts (host included).

### Crash fixes / hard lessons learned
- **Named entity cache double-free**: fixed by ensuring `named_entities_DO_NOT_USE` stores the owning `shared_ptr` (never create a new control block from raw `Entity*`).
- **Dynamic model callback crash on load**:
  - `HasDynamicModelName` contains runtime-only `std::function` glue. Its serializer currently only serializes the base component (it does **not** persist `dynamic_type`, `base_name`, or the fetcher), so loaded entities will have an uninitialized/empty dynamic-model-name component unless we reconstruct it.
  - Current approach: **post-load reinitialization pass** (`reinit_dynamic_model_names_after_load()`) removes and recreates dynamic model-name fetchers for known entity types.
  - `HasDynamicModelName::fetch()` has guards for uninitialized / missing fetcher (returns the base name and logs a warning) to avoid hard crashes.
  - **Known gap**: `FruitJuice` dynamic model name still can’t be reconstructed correctly (missing persisted subtype/state).
  - **Known gap (CLI path)**: the `--load-save <path>` load path currently applies the snapshot but does **not** run the post-load dynamic-model-name reinit pass (slot-number loads do).

### Still to do (Phase 1 follow-ups)
- Expand slot count to **12** (or confirm desired count) and finalize room layout.
- Replace temp planning save trigger placement/logic with the final intended station (current is marked TODO to remove).
- Ensure the CLI `--load-save <path>` code path also runs the post-load reinit (same as slot loads) to avoid missing dynamic model names.
- Add remaining header metadata (optional but useful): `display_name` and `playtime_seconds` are not filled yet in Phase 1.
- Clarify snapshot-vs-hybrid scope: Phase 1 snapshot saves **all entities** (including transient items); Phase 2 should narrow to a stable, intended “hybrid delta” subset.

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
- **Hybrid scope v1 (design intent)**: **Furniture placements only**. Note: Phase 1 snapshot implementation currently saves **all entities** (including transient items); Phase 2 should narrow this to an intended delta.
- **Future scope**: likely expands toward “C” (broader saveable set), but start narrow.
- **Saving UX**: both **diagetic Save Station** + **menu button** (enabled only in Planning).
- **Delete UX**: **toggle delete mode** + select slot (no separate delete pedestal).
- **Thumbnail previews**: prefer **minimap-style preview** (similar in spirit to seed UI), with a **screenshot fallback** if minimap render is non-trivial.

## Goals
- **Load an existing save file** and reconstruct the **authoritative world/map** to match it.
- Ensure loading does **not** get overwritten by procedural generation (`ensure_generated_map`).
- Add a **diagetic UI** (in-world room) to browse save slots “PS2-style” and load one.
- Add a **replay mode** that can start from a save, run deterministic input replay, and **validate** the world state.

## Current Code Reality (what exists today)

### Serialization foundations already exist
- `Entity` serializes ids, tags, and polymorphic components (`src/entity.h`).
- `Map` serializes the world entity snapshot + map metadata (`src/map.h`).
- Networking already serializes `Map` inside `ClientPacket::MapInfo` (`src/network/serialization.h`).

### Authoritative apply exists (Phase 1 snapshot install)
- Server-only save/load helpers live in `src/client_server_comm.cpp`:
  - `server_only::save_game_to_slot(int)`
  - `server_only::load_game_from_slot(int)`
  - `server_only::delete_game_slot(int)`
- Save/load file IO + header-only reads live in `src/save_game/save_game.*`.
- The apply path installs the snapshot into the authoritative server state, sets `was_generated = true`, invalidates caches, and forces a map sync.
- Important note: `Map::update_map()` is still minimal, but Phase 1 does not rely on it; we directly replace `Map::game_info` on the authoritative server map.

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
- magic: `"PHARMSAVE"`
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
- Phase 1 can ship using a full snapshot payload (`Map`) to prove correctness end-to-end.
- Phase 2 refactors to true hybrid delta (filter + stable identifiers), once load/apply is solid.

## Authoritative Load: Apply Save → World Reconstruction

### Non-negotiables
- Load must run **server-side** (authoritative) so the world is consistent for multiplayer and for replay.
- Loading must ensure `Map.was_generated = true` so the generator doesn’t wipe the loaded world.
- After installing entities, we must invalidate caches (named entity cache, walkable/path caches).
- **Host-only**: only host server thread can apply saves; clients are synced afterward.

### “Apply Save” algorithm (server thread)
1. Read save header and payload from disk.
2. **Phase 1 (implemented)**: install snapshot directly:
   - Replace authoritative entity list with the saved snapshot
   - Set `was_generated = true` so procedural generation doesn’t wipe it
   - Update server map fields (`seed`, `showMinimap`, etc.)
   - Reinit runtime-only callbacks that aren’t serialized (ex: `HasDynamicModelName` fetchers)
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
In `Map::generate_lobby_map()`:
- Add a new `IsTriggerArea` similar to existing lobby triggers:
  - `IsTriggerArea::Lobby_LoadSave` (new)
  - Transition to new game state: `game::State::LoadSaveRoom` (new)

### Room generation
Add `Map::generate_load_save_room_map()` (pattern after `generate_model_test_map()`):
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
- Server loads and applies the save (authoritative) **or deletes the slot**, depending on delete mode.
- Clients receive sync.
- Transition to **Planning** and place players at planning spawn.

### Delete behavior (v1)
- In the room, provide a **Delete Mode toggle**.
- When Delete Mode is ON, selecting a slot pedestal **deletes** it (empty slots stay grey).
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
- ✅ Implement save header + slot enumeration + delete
- ✅ Implement “apply loaded world” using a full snapshot payload (fast correctness)
- ✅ Add Load Save room + lobby trigger + transitions
- ✅ Add planning-only save restriction + user feedback (toast/announcement)
- ✅ Add CLI `--load-save` hook
- ⛔ Replay end-state validation hooks (still pending)

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

