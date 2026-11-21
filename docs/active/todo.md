---
kanban-plugin: basic
---

# Pub Panic! Task Tracker

**Quick Stats:**
- Priority Fixes: 7 bugs + 4 UX issues
- Core Features: 6 items
- Save/Load: 6 items
- ECS Refactor: 4 items
- Afterhours Migration: 10 items (8 done)
- Afterhours Vendor Move-Over: 33 items (1 done)
- Infra Changes: 15 items
- Code Health: 22 items
- Low Priority: 14 items
- Undecided: 6 design questions
- Completed: 28 items

---

## priority fixes (bugs)

- [ ] mojito model is big square
- [ ] mai tai has no model
- [ ] pathfinding crashes when rendering the waiting queue (disabled for now)
- [ ] lime doesnt want to go into drink when cup is in register
- [ ] lime wont go in unless i add lime juice first?
- [ ] toilet hitbox is messed up
- [ ] vomit hitbox is hard especially without mop

## priority fixes (UX)

- [ ] Text doesnt rotate based on the camera (should billboard properly)
- [ ] Not clear you can cycle through alcohols (add UI hint)
- [ ] settings dropdown doesnt respect selected language (i18n bug)
- [ ] add confirmation dialog when switching resolution and languages

## core features

- [ ] Particle system (effects for drinks, vomit, etc.)
- [ ] Tile sheet support
- [ ] Spritesheet animator
- [ ] Create Doors (visual only - animate when walked through, no gameplay effect)
- [ ] Practice mode to learn new recipes (not just tutorial)
- [ ] First round patience should be 2x (gentler onboarding)

## save/load follow-ups

- [ ] Expand save slot count to 12 and finalize room layout
- [ ] Replace temp planning save trigger with final Save Station design
- [ ] CLI `--load-save <path>` should run post-load reinit pass (like slot loads do)
- [ ] Add header metadata: `display_name` and `playtime_seconds`
- [ ] Phase 2: narrow snapshot to hybrid delta (furniture only, not transient items)
- [ ] FruitJuice dynamic model name can't be reconstructed after load (missing persisted subtype/state)

## ecs refactor tasks

- [ ] Review `CanHold*` components for consolidation (see archived ECS plan)
- [ ] Replace `optional<vec2>` with Location typedef in AI targeting
- [ ] Finalize AI data split: what belongs in `IsCustomer` vs per-state components
- [ ] Validate AI split against planned features (thief/VIP/karaoke)

## infra changes

- [ ] BUILD PERF: Optimize magic_enum usage (accounts for 350+ seconds of template instantiation)
- [ ] collision for player to change to cylinder
- [ ] pressing two movement at the same time while moving camera sometimes feels weird
- [ ] In pause menu, remap key bindings in layer for arrows keys to choose options
- [ ] rendering order for text background is weird
- [ ] likely dont need to queue network packets since we always send the full state
- [ ] AttachmentSystem: unify held item/furniture/handtruck/rope via generic parent→child attachment
- [ ] Progress/Cooldown components + ProgressRender: centralize timers/progress bars
- [ ] TriggerAreaSystem: own entrants counting, validation, progress/cooldown and activation
- [ ] GameState change hooks (StateManager on_change)
- [ ] Entity Tags/Groups (+ TaggedQuery): bitset tags like Store, Permanent, CleanupOnRoundEnd
- [ ] EntityRegistry (id → shared_ptr): O(1) lookup for ids
- [ ] CollisionCategory/CollisionFilter components: data-driven collision layers
- [ ] PrefabLibrary (data-driven entity builders): move repetitive makers to JSON prefabs
- [ ] BillboardText/WorldLabel component + system

## afterhours migration (in progress)

Pointer-free serialization migration - see `docs/active/afterhours_pointer_free_next_steps_plan.md` for full context.

**Phase 0 - Prep:**
- [ ] Remove/gate debug `printf`s in `src/network/serialization.cpp`

**Phase 1 - Handle system in afterhours:** ✅ DONE
- [x] Implement `EntityHandle` + slots/free-list/dense-index in `vendor/afterhours`
- [x] Add `id_to_slot` and make `getEntityForID` O(1)
- [x] Rewrite cleanup/delete paths to keep slots/dense indices correct and bump generation
- [x] Add `EntityQuery::gen_handles()` and `gen_first_handle()`

**Phase 2 - Migrate pharmasea:** ✅ DONE
- [x] Audit and migrate remaining serialized entity relationships to handles
  - can_hold_item.h: EntityRef for held_item, last_held
  - is_squirter.h: EntityRef for held_item, held_drink
  - adds_ingredient.h: EntityRef for parent
  - has_rope_to_item.h: std::vector<EntityRef> for rope
  - has_waiting_queue.h: std::array<EntityRef> for ppl_in_line

**Phase 3 - Snapshot DTOs:** (mostly done)
- [x] Add snapshot DTO layer (`world_snapshot_blob`)
- [x] Switch save-game to snapshot payloads
- [x] Remove `PointerLinkingContext` from network and save contexts
- [ ] Add save versioning + migration hooks

**Decisions to lock down:**
- [x] Handle layout: prefer `uint32_t slot/gen` for stable wire format
- [ ] Temp entity behavior: "no handles until merge" vs "handles on create"
- [ ] Save/load fixups: define when to resolve references after load

## afterhours vendor move-over (consolidated)

Consolidated from the former gaps + onboarding sections, `afterhours_gaps.md` (G-E/G-M), `docs/plans/afterhours-onboarding-report.md` (bump `4c86b3a` -> `4d8da9e`), `docs/plans/afterhours-main-upgrade.md`, and `fsm_todo.md`. Tags: (a) move game code into afterhours, (b) adopt existing afterhours feature instead of local code, (c) delete local code after upstream replacement. Pointer-free/handle work stays in the migration section above; FSM conversions stay local (see last item).

**Prerequisite:**
- [ ] (b) BLOCKER: fix `VALIDATE` undeclared build break (pch defines `AFTER_HOURS_REPLACE_VALIDATE`, `src/log/log.h` provides none; new vendor uses it in `core/system.h` + `input_system.h`) — vendor cannot be consumed until this lands. First step: include `engine/assert.h` before vendor headers, then build + `./pharmasea.exe -t` + one .e2e script

**Adopt upstream, then delete local (b/c):**
- [x] (b/c) Frustum: replace vendored `src/engine/frustum.h` (raylibExtras copy) with `plugins/frustum.h` (`make_frustum` + sphere/box tests) in rendering_system — done: `plugins/frustum.h` included via `src/ah.h`; the per-frame refresh builds the frustum from `MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection())` in `render_manager::on_frame_start()`, `should_cull` uses `intersects_box`, and `src/engine/frustum.h` is deleted
- [x] (b) Instancing: pilot `InstanceBatcher`/`draw_model_instanced` (`plugins/models.h`) — done as a full-path pilot: unlit WHITE model copies in `draw_transform_with_model` now batch per model into a static `InstanceBatcher` (transform composed exactly as `DrawModelEx`: scale x Y-rotation x translate) and flush at the end of `Map::onDraw`; tinted/highlighted/ghost and lighting-shader copies keep the `DrawModelEx` path (no per-instance tint upstream — G-P8). ModelLibrary unchanged. Verified: build + unit tests, `13`/`02`/`15` e2e, and a before/after screenshot at the register area is visually identical
- [x] (b) LOD: add `plugins/lod.h` hysteresis LOD alongside frustum culling — done as model->box impostors (no LOD model variants exist in resources): `render_model_normal` tracks distance from the camera target per entity in `ModelRenderer::lod_level` via `afterhours::Lod::update_sq` (threshold 26 tiles, upstream margin), and far non-character entities with a `SimpleColoredBoxRenderer` fall back to the box draw; characters and near models unchanged. Payoff is bounded by design here — the follow camera keeps most geometry near, and frustum culling + instancing already cover the large costs; the value is the flicker-free switch point if variant assets ever land. Verified: build + unit tests, `13`/`02`/`03` e2e, register-area screenshot identical to pre-LOD
- [ ] (b) Particles: adopt `particles::Emitter` for drink/vomit effects — closes the core-features particle item with a tested pool. First step: one drink-effect emitter, no new hand-rolled system
- [x] (b) RNG: evaluated `RngStream` for map generation — SKIP (filed as G-P12): upstream seeds strings with `std::hash<std::string>` and draws via `std::uniform_*_distribution`, both STL-implementation-defined, so seeded maps would differ across compilers; local pcg32 + FNV-1a seed is portable bit-for-bit (what `run_determinism.sh` and host/client agreement rely on). Per-domain stream isolation is the one real benefit; revisit only as a local pcg stream class. Found en route (not fixed; changing them re-seeds every map): `RandomEngine::get_float(a,b)` computes `(a + (b - a)) * dist` = `b * [0,1)`, ignoring `a`; `get_int` mods a fixed 1..1000 distribution, so ranges wider than 1000 never reach their top
- [ ] (b) Tests/files: consider `files::get_cache_path()` for regenerable data. First step: none yet
- [x] (b) Tests/files: `files::TestRoots` — done: vendor `files.cpp` now compiles in, `Files::game_folder()` redirects under `active_test_roots`, and `test_files_test_roots` proves writes land in the temp root and restore after (game sago include became declarations; upstream copy owns the definitions)
- [ ] (b) Settings input: use `input::Repeat` for held repeat; enum `is_held/is_pressed` for new code. First step: none yet
- [x] (b) Settings input conflict check — done: `KeyMap::conflicts_in` converts the layer to upstream form and queries `input::actions_using`; `set_mapping` (the remap screen's write path) logs and refuses a binding already used by another action. Covered by `test_keymap_conflicts`. Note the remap popup itself is still disabled in `settingslayer.cpp`, so this guards the API until the popup returns
- [ ] (b) Fixed tick: confirm fixed-tick wiring, then set `SystemManager::max_fixed_ticks` catch-up cap — prevents spiral-of-death hitches. First step: find the tick path (no `fixed_tick` refs in `src/` today)
- [ ] (b) Opportunistic: upstream `src/image.h` (`image_size`/`decode_image`), `font_size_per_point`/`load_font_from_memory`. First step: none yet
- [x] (b) `FrameRateStats` readout in e2e runs — done: `App::note_frame` feeds `afterhours::FrameRateStats` + a 1024-frame p99 ring; `e2e_manager` registers upstream `perf_commands` (`expect_fps_above`/`expect_p99_below`) with that provider and logs the readout at script finish (G-E5 closed)
- [x] (b) Arena allocators — evaluated, DEFER (filed as G-P14): upstream `memory/arena.h` is a standalone bump allocator with no EntityQuery integration, and local queries return owning `RefEntities` vectors whose lifetimes escape any frame arena (~102 `.gen()` call sites). A trial means changing `EntityQuery` result ownership engine-wide — do not do blind. First step if revisited: Tracy allocation profiling on one hot system to prove allocation is the cost
- [ ] (b) Query API: compare our `EntityQuery` to vendor query APIs; use `add_filter(lambda)` for new queries, leave existing `add_mod()` sites — avoids new `Modification` subclasses. First step: one new query in lambda form
- [x] (b) System hooks: add `should_iterate() { return false; }` to the once-only systems — already done: every once-only system has it (`OnFrameStartSystem`, `GenerateStoreOptionsSystem`, `CleanUpOldStoreOptionsSystem`, `RenderWalkableSpotsSystem`); the remaining `once()` systems (`RenderEntitySystem`, both trigger-entrant counters, `ResetCustomersThatNeedResettingSystem`) do real per-entity work in `for_each_with`, so they must keep iterating
- [x] (b) Pathfinding — evaluated (API diff done), DEFER (filed as G-P13): same design both sides (async request/response queue + `is_walkable` callback + AStar/BFS). Differences that block a drop-in: upstream paths on its `Vector2Type` and drives movement from its own systems, while local `CanPathfind` is serialized (parent/path_size), resolves via `PathRequestManager` on a worker thread, and movement/collision lives in the component; local search also snaps to `TILESIZE` grid via `vec::forEachNeighbor`. Migration is a rewrite of `CanPathfind`, not a swap — only with a parity test on `test_pathing` maps, and no current pain justifies it
- [x] (b) Timers — evaluated, SKIP (see G-P11): `HasWork` is progress-% + work callback, not a timer at all. `CooldownInfo` maps field-wise to `HasTimer` (`reset_to`↔`reset_time`, `remaining`↔countdown remainder, `enabled`↔`!paused`) but `HasTimer` counts up, has no zpp `serialize`, and is a standalone component where `CooldownInfo` is an embedded struct — adopting it drops timer state from saves and churns component layout for no gain. Keep local; the Progress/Cooldown infra item should build on `CooldownInfo`
- [x] (b) E2E time-scale: adopt upstream `--time-scale`/`--e2e-speed` (G-E2) — done: parsed via upstream `parse_e2e_args` into `E2E_TIME_SCALE`; both the client (`App::run`, via the `App::update`/`App::draw` split) and the server's fixed 120 Hz tick (`Server::run`) *substep* at real dt instead of enlarging dt, so the sim follows the exact 1x trajectory (dt-multiply broke customer cleanup at 4x, and client dt-multiply broke lobby key-holds); substepping is `#ifndef NDEBUG` only, release builds keep the plain single-step paths. `tests/e2e/run_e2e.sh` forwards `PHARMASEA_E2E_TIME_SCALE`. Verified: full 12-script suite passes at 2x, `03` passes at 4x (31s -> 16s), 1x unchanged. Ceiling: 10x saturates this machine in a debug build (server snapshot sync + GNS starve); at 4x `16`'s final return-to-lobby assert can lose the race against the bypass lobby W-pulse (pre-existing bounce flake) — run the suite at 2x
- [x] (b) Headless: adopt the NONE backend (`DisplayMode::Headless`, G-E6) — done: `App` routes `AppSettings::headless` (the `--headless` flag, and the e2e default) to upstream `graphics::RaylibHeadless` instead of `InitWindow` — no window is created; drawing goes to the backend render texture behind the existing `mainRT`, so e2e screenshots keep working, and the frame clock is real elapsed time. Two raylib headless traps were fixed game-side (G-P10): intro/loading text draws are skipped headless (no default font without a window), and `FontLibrary` redoes the font load manually when `LoadFontEx` leaves the atlas unuploaded. Windowed path unchanged (`--windowed` control passes). Verified: unit tests, fast batch fully headless, day screenshot visually correct

**Move game code upstream (a), then delete local stopgap (c):**
- [ ] (a) UI offscreen warning system ("purpling" detection) — generic UI concern, belongs in the library. First step: spec the warning API from pharmasea's UI call sites
- [ ] (a) Entity key subscription (track which entities want which keys) — input routing belongs with the vendor input system. First step: list current key-consumer call sites
- [ ] (a/c) E2E interaction: semantic walk-to/interact commands on `input_injector` (G-E3) — local `hold_key`/`walk_to` stopgaps are brittle across client/server positions. First step: upstream `walk_to` prototype; delete stopgaps once authoritative confirm lands
- [ ] (a) Test command channel: general queued main-thread -> server command channel (G-M1; save leg already solved game-side via the `SaveSlot` packet) — one packet type per verb does not scale. First step: design the queued verb + completion signal in the multiplayer plugin shape (`vendor/afterhours-online`)
- [ ] (a) Player query: host/client-symmetric player count (G-M2, unblocks `expect_player_count` + network scripts `09`/`10`) — players are not in the client `EQ` collection today. First step: document one query that works on both sides
- [ ] (a) Seed entry: pre-lobby `--seed` entry point in the plugin session config (G-M3) — CLI seed never reaches the authoritative map today. First step: host applies CLI seed at server start
- [ ] (a) Two-process runner: host+client orchestration for scripts `09`/`10` (G-M4; pumpkin `tests/online_e2e.py` is the reference). First step: shell runner that spawns two processes on a fresh port
- [ ] (a) Server state copy: server-local `MenuState`/`GameState` copy (G-M5) — singletons are currently shared/raced across threads. First step: plugin-owned server copy with a race-free test read
- [ ] (a) Progress sync: serialize per-customer progress (`num_orders_had`, G-M6) via plugin state sync, or document it host-only — client mirrors always read 0 today. First step: decide serialize-vs-document with save owners

**Vendor hygiene (c, local cleanup):**
- [ ] (c) Includes: route all vendor includes through the local wrapper `src/ah.h` and stop including vendor internal headers (merged former pair) — one include surface to bump. First step: grep direct `afterhours/src/` includes outside `ah.h`
- [ ] (c/b) Serialization: centralize serialization on vendor implementations and standardize on `afterhours::Entities`/`afterhours::RefEntity` aliases (merged former pair) — deletes local alias/serialization drift. First step: inventory local aliases in `src/entity.h`
- [ ] (a/b) Evaluate migrating `EntityHelper` functionality to vendor; review vendor job/task facilities and component-registration utilities (merged former trio; evaluate only) — each may delete local helpers if upstream fits. First step: one-page API comparison per facility, no code
- [x] (c) Submodule: `vendor/afterhours` initialized and pinned — verified: `.gitmodules` present, gitlink and checked-out HEAD both `4d8da9e08c46d0c7f2a7004b746b7b31e28bdadd`

**Windows cross-compile (`zig build -Dtarget=x86_64-windows-gnu`):**
- [ ] BLOCKED on two vendored libs (fetch/build at home, drop in `vendor/`):
  - Windows raylib 5.5: `vendor/raylib/libraylibdll.a` + `raylib.dll`. Must be 5.5 — afterhours calls 5.5 APIs (`IsKeyPressedRepeat`, `DrawRectangleRoundedLinesEx`, `GetShapesTexture`); the vendored `vendor/raylib/*.h` are 4.6-dev and too old even for headers.
  - Windows GameNetworkingSockets: `vendor/libGameNetworkingSockets.dll` + its import lib. Must be a **mingw/zig-built** build, not MSVC: pharmasea consumes it through the C++ `ISteamNetworkingSockets` virtual interface, so the libc++ ABI has to match zig's. The vendored copy today is the macOS `.dylib` only.
- [x] Game-side fixes already landed for the probe: `M_PI` removed from `src/engine/util.h` (`PI_F`), and `build.zig` defines `WIN32_LEAN_AND_MEAN`/`NOGDI`/`NOUSER`/`_USE_MATH_DEFINES` for Windows (raylib↔windows.h `CloseWindow`/`ShowCursor` clash). Also: vendored `backward.hpp` includes `<psapi.h>` before `<windows.h>`, so `game.cpp`/`crash_handler.cpp` include `<windows.h>` first under `_WIN32`. After these, `src/game.cpp` compiles clean to a Windows object (`zig c++ -target x86_64-windows-gnu -c`); only the link libs below are missing.
- [x] Upstream gaps from `~/p/port_to_zig_build.md` rechecked at pin `4d8da9e`: all three already fixed (`arena.h` `_aligned_malloc` branch, `headless_gl_windows.h` exists, backend `ExportImage` uses `path.string().c_str()`). Remaining blockers are the two libs above, not afterhours source.

**Done / not moving:**
- [x] E2E: adopt upstream results JSON (`write_results_json`/`--results-json`) + `testing::is_e2e_running()`, then typed `register_command`; keep hand parser in e2e_manager.h until then — done: hand parser deleted, scripts use `wait_until`, pending commands isolated in the singleton default collection (client map generation wipes its own collection)
- [ ] Skip (recorded in report): imm UI suite, house_of_cards (revisit after instancing), lit shader, sound wrappers, toast plugin (local `toastmanager.h` stays), query indexes, expect_drawn capture asserts, KeyChord input, auto gamepad-mapping load, separate UI entity collection
- [ ] Keep local (FSM, per `fsm_todo.md` §3/§7): M01-M24 explicit-FSM conversions (`transition_to()` tables, derived phases) stay in pharmasea; vendor afterhours stays untouched — only consume its `System`/`should_run` patterns. First step: none; revisit only if a generic FSM helper emerges from two+ machines

## e2e coverage expansion

Today: 1 script (`tests/e2e/01_bypass_to_lobby.e2e`, wait-only, no assertions), `e2e_manager.h` understands only `wait`/`wait_frames`/`key`, and `-t` unit tests cover random/ui-widget/serialization/pathing/map-playability. Scripts run via `./pharmasea.exe --e2e <file>` or `--e2e-dir`. Enablers first — most scenarios below are one script once assertions exist.

**Harness enablers (do first):**
- [x] Add assert commands to `e2e_manager` dispatch: `expect_menu_state <Root|Network|Settings|Game>`, `expect_game_state <InMenu|Lobby|InGame|Paused|LoadSaveRoom>` (read `MenuState`/`GameState` singletons; fail script + nonzero exit, not just log) — asserts poll until `within <s>` deadline; also `expect_order`, `expect_money`, `expect_setting`, `save_slot`, `set_setting`/`save_settings`/`reload_settings`, `screenshot`
- [x] Add `expect_day <n>` + `expect_phase <day|night>` asserts (`HasDayNightTimer::days_passed()`/phase, same read the `completed_first_day` replay spec uses)
- [x] Add `expect_entity <ComponentName> <op> <count>` assert via `EQ().whereHasComponent<...>` for a small registered set (customers, drinks, furniture) — spawn/serve/cleanup smoke without UI text — sets: Customer (`IsCustomer`), Drink (`IsDrink`), Furniture (`CanHoldItem`), Player (type tag; reads 0 on client thread, see `afterhours_gaps.md` G-M2)
- [x] Make failures fatal and legible: unknown command and failed assert exit nonzero with script name + line (today unknown commands only `log_warn` and the run still passes); print `[PASS]`/`[FAIL] <script>` summary per script in `--e2e-dir` mode — exit code plumbed via `E2E_EXIT_CODE`; `@xfail` scripts are skipped in dir mode
- [x] Screenshot-on-failure: capture one screenshot next to the script on assert failure/timeout so CI failures are debuggable without rerunning locally — via texture export; `raylib::TakeScreenshot` drops directories on macOS (G-E4)
- [x] Adopt upstream afterhours results JSON (`--results-json`, see afterhours onboarding) once the above lands, so `--e2e-dir` output is machine-readable in CI — harness migrated to upstream `E2ERunner` + `register_command`; `--results-json` now emits the upstream schema

**Core loop scripts (need only the enablers above):**
- [x] `02_bypass_to_ingame.e2e` — extend 01 past lobby: bypass reaches `game::State::InGame`, player entity exists at spawn (bypass_helper already holds W in lobby/in-game) — asserts InGame + furniture replicated; player probe blocked on G-M2
- [x] `03_full_day_cycle.e2e` — `--bypass-rounds 1`: day starts, customers spawn, night falls, day counter increments to 1, transient entities cleaned up (assert day + customer count returns to 0) — `@slow`, verified passing (~200s real time; G-E2 would speed it up)
- [x] `04_pause_resume.e2e` — in game: `key ESCAPE` -> `Paused`, again -> `InGame`; assert no day-timer advance while paused — pause/resume verified; timer-freeze assert still implicit (no `expect_day` delta check)
- [x] `05_settings_roundtrip.e2e` — open Settings from Root, toggle a setting, back to Root, relaunch path: setting persists (assert via settings file/`settings::Data`, needs one `expect_setting` assert or a restart-and-check script pair) — covers the persistence path via `set_setting`/`save_settings`/`reload_settings`; UI-click variant waits on G-E1/G-E3
- [x] `06_customer_serve_loop.e2e` — seeded (`--seed e2e-serve`): customer arrives, orders, is served, leaves happy; assert order state transitions + money/day stats changed (hardest script; split into spawn/order/serve asserts and mark `@slow`) — spawn + `Ordering` verified `@slow`; the full serve leg now lives in `13_first_customer_served` (walk_to/hold_key verbs, drink built on the register); money asserted in `14`

**Determinism, save/load, replay:**
- [x] `07_seed_determinism.e2e` — same `--seed` twice via `--e2e-dir` harness or shell pair: identical map hash + first-customer spawn (needs `expect_map_hash` or position assert; guards the pcg seed path the `-t` random tests only cover at unit level) — done as `tests/e2e/run_determinism.sh` over `--generate-map` (10 seeds, byte-identical pairs); in-game seed pinning blocked on G-M3
- [x] `08_save_load_roundtrip.e2e` — in game day 1: trigger save slot write, `--load-save <path>` relaunch, assert day/phase/furniture count match pre-save (covers CLI `--load-save` reinit follow-up already in save/load section) — `@slow`; save fires via new `SaveSlot` packet (`Client::send_save_slot_request` -> `Server::queue_packet` -> server thread `save_game_to_slot`), asserted with `expect_save_slot`; pair verified (write slot 9, `--load-save 9` relaunch lands on saved world)
- [x] Add replay specs next to `completed_first_day` in `src/tests/replay_specs.cpp`: first customer served, first purchase placed, day-1 cleanup empty — replay validation today has exactly 1 spec — superseded: recorded replays retired (specs, `recorded_inputs/`, `--replay`/`--record-input` flags, and the `make test` replay line all removed); cleanup is covered by `03`, serving by `13_first_customer_served` (verified end-to-end), and purchasing by `14_first_purchase_placed` (serve two, buy the seeded $5 Table on day 2; calibration also found and fixed store stock being deleted by its own cleanup system in the same transition tick)
- [x] Record one new `.txt` replay per spec (`recorded_inputs/`) with the recording command documented in the file header, so specs don't silently rot when inputs change — closed by the switch to e2e commands above; no recordings remain in the repo

**Network (loopback, single machine):**
- [ ] `09_host_lobby_loopback.e2e` — host creates lobby (`expect_game_state Lobby`), second process as client with `--username e2e-client` joins, host sees 2 players, client reaches Lobby (two-process script; needs a small shell runner under `tests/e2e/run_network.sh`, not the in-game parser) — deferred to the network milestone; needs G-M2 (player count probe) + G-M4 (two-process runner)
- [ ] `10_client_settings_isolation.e2e` — regression for the fixed "client can't change settings / host overrides menu state" bugs: client changes a local setting in lobby, assert host menu state unchanged — deferred with `09`; single-process half is covered by `05_settings_roundtrip.e2e`

**Negative + robustness:**
- [x] `11_corrupt_save.e2e` — `--load-save` on a truncated/garbage slot file: game reaches menu, logs the load failure, does not crash (assert exit code 0 + menu state) — fixture at `tests/e2e/fixtures/corrupt_slot.bin`, verified exit 0
- [x] `12_unknown_key_script.e2e` — script with a bogus `key` name and a bogus command fails fast with the line number (locks in the fatal-failure enabler above) — `@xfail`, verified exit 1 naming file + line 7, skipped in dir mode
- [x] Map-gen sweep as e2e: `--generate-map` across 10 fixed seeds, assert every map passes the existing `test_map_playability` checks headlessly (move that check behind a CLI-reachable assert if needed) — sweep covers generate-without-crash + determinism in `run_determinism.sh`; playability stays unit-level until a CLI-reachable assert exists

**FSM follow-ups (found during the FSM explicit-state pass):**
- [x] Add an e2e script for the load/save room delete-mode toggle — `tests/e2e/16_loadsave_delete_mode.e2e` (`@slow`): walks lobby -> load/save room, toggles delete mode on/off/on via the pad (new `expect_delete_mode` assert on `HasLoadSaveMode`), creates slot 8 via `save_slot`, then deletes it via the slot pad (new `expect_no_save_slot` assert); passes in ~31s (one abort in 3 runs was the known GNS service-thread flake under `nice`, after all steps completed)
- [x] `network::Client::id` / `SessionState::InGame` — investigated: `id` is assigned on `PlayerJoin` (`is_you`) in `src/network/client.cpp`, so `InGame` is reachable; the actual gap was that disconnect never cleared `id`, so the derived state would keep reporting `InGame` after a lost connection — fixed by clearing `id` in `Client::tick`'s disconnect branch

**CI/perf hygiene:**
- [x] Add `make e2e` target: rebuild if needed, run `--e2e-dir`, surface per-script PASS/FAIL + exit code (today e2e is manual-only) — `make e2e` (fast set) and `make e2e-slow`, one process per script via `tests/e2e/run_e2e.sh`
- [x] Tag slow scripts (`03`, `06`, `09`) so a fast subset (`01,02,04,11,12`) can run per-commit and the full dir nightly — `@slow`/`@xfail` header tags parsed; `run_e2e.sh` excludes both by default (`12` is `@xfail`, runs standalone)
- [x] Frame-time smoke: `03_full_day_cycle` ends with `expect_fps_above 10` / `expect_p99_below 200` (upstream `perf_commands` fed by `App` frame stats, G-E5) — catch perf cliffs, not frame-perfect timing; measured 74 fps / p99 25ms at 2x

## code health

- [ ] Create a precompiled header (pch.hpp) for heavyweight third-party headers
- [ ] Split src/engine/graphics.h into graphics_types.h and graphics.cpp
- [ ] In src/engine/model_library.h, forward declare Model and move includes to .cpp
- [ ] Introduce a lightweight network/fwd.h or network/api.h
- [ ] Run Include-What-You-Use (IWYU) across src/
- [ ] Reduce transitive includes in public headers
- [ ] Avoid including <raymath.h> and <rlgl.h> from headers
- [ ] Isolate template-heavy headers (bitsery, serialization)
- [ ] Add -ftime-trace to profile compile hotspots
- [ ] Enable sanitizers in Debug builds: ASan + UBSan
- [ ] Add clang-tidy with bugprone-*, clang-analyzer-*, etc.
- [ ] Turn warnings into errors in CI (-Wall -Wextra -Wshadow etc.)
- [ ] Strengthen types: replace naked int/float with enum class and strong typedefs
- [ ] Prefer std::unique_ptr/std::shared_ptr for ownership
- [ ] Add RAII wrappers for raylib resources (Model/Texture/Sound)
- [ ] Use tl::expected for fallible operations
- [ ] Apply [[nodiscard]] and noexcept where appropriate
- [ ] Add unit tests for serialization/deserialization
- [ ] Add guard helpers for index/bounds checks
- [ ] Add a crash handler with backward-cpp
- [ ] Document include policy and code style
- [ ] Audit and minimize global/singleton usage

## low priority

- [ ] Resource export for binary packaging (single-file distribution)
- [ ] Nav mesh for walkability (later for endgame with many customers)
- [ ] Upgrade A* to Theta* (smoother diagonals)
- [ ] Consider raysan5/rres for resources
- [ ] 0xABAD1DEA for static globals
- [ ] Look into FMOD for sound
- [ ] Investigate Fiber jobs (Naughty Dog style)
- [ ] Flatmap/flatset for cache locality
- [ ] Ability + Cooldown pattern for interactions
- [ ] LifecycleSystem (deletion/cleanup policies)
- [ ] EventBus (GameEvents)
- [ ] TransformFollower sockets (named anchors)
- [ ] InputContext system (Menu/Game/Paused)
- [ ] Pause menu with textual options (rethink entire menu UI)

## undecided (need design discussion)

- [ ] Simple Syrup doesn't disappear after one use (intentional or bug?)
- [ ] Penalty for wasting ingredients? (tie to "clean at end of day"?)
- [ ] Should alcohol be multi-use bottles? (fundamentally changes gameplay)
- [ ] Why should you clean up the bar? (need clear consequence)
- [ ] Not enough customers to need automation? (balance question)
- [ ] What is the "one main gameplay item"? (core loop discussion)

## no repro

- [ ] cosmopolitan model is invisible
- [ ] PS4 Controller touchpad causing "mouse camera rotation"

## completed

- [x] joining twice from a remote computer crashes the host
- [x] drop preview box sometimes has the wrong color
- [x] bug where you cant place the table next to the register
- [x] Cant repro but i got the FF box to show trash icon inside
- [x] remove job system and switch to components
- [x] add reroll to shop
- [x] need preview for where item will go
- [x] Tell the player how many customers are coming this round
- [x] hard to tell that a new machine/stockpile has been spawned
- [x] at round 3 the people got stuck in line
- [x] highlight spots on the map where this thing can go
- [x] day 3 doesnt work
- [x] During planning its hard to know what each machine is
- [x] default language is reverse which is confusing
- [x] vomit is broken not working
- [x] Add purchasing medicine cab
- [x] Roomba keeps getting stuck at exit
- [x] Hide pause buttons from non-host
- [x] Client player cant change settings
- [x] you can fill up the cup while its in the cupboard
- [x] Progression screen should show new drink's recipe
- [x] if you take the drink back from the customer you crash
- [x] Automatically teleport new players when joining
- [x] controls dont work for gamepad in settings during game
- [x] warn player when deleting required items
- [x] show incoming customer count/timing UI
- [x] corner walls (fixed by wall mesh change)
- [x] raylib intro card (powered by raylib splash)

%% kanban:settings
```
{"kanban-plugin":"basic"}
```
%%
