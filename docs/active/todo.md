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
- Afterhours Gaps: 12 items
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

## afterhours gaps

Track features that should be in afterhours library rather than pharmasea:

- [ ] UI offscreen warning system ("purpling" detection)
- [ ] Entity key subscription (track which entities want which keys)
- [ ] Arena allocators (already in afterhours, evaluate usage)
- [ ] vendor/afterhours: switch all includes to local wrapper `src/ah.h`
- [ ] vendor/afterhours: stop including internal headers
- [ ] vendor/afterhours: centralize serialization on vendor implementations
- [ ] vendor/afterhours: standardize on `afterhours::Entities`/`afterhours::RefEntity` aliases
- [ ] vendor/afterhours: evaluate migrating `EntityHelper` functionality to vendor
- [ ] vendor/afterhours: compare our `EntityQuery` to vendor query APIs
- [ ] vendor/afterhours: review job/task facilities
- [ ] vendor/afterhours: check for vendor component-registration utilities
- [ ] vendor/afterhours: ensure submodule is initialized and pinned

## afterhours onboarding (2026-11 bump 4c86b3a -> 4d8da9e)

Full report: `docs/plans/afterhours-onboarding-report.md`. Submodule pulled to `4d8da9e`; no src changes made yet.

- [ ] BLOCKER: fix `VALIDATE` undeclared build break (pch defines `AFTER_HOURS_REPLACE_VALIDATE`, `src/log/log.h` provides none; new vendor uses it in `core/system.h` + `input_system.h`) — include `engine/assert.h` before vendor headers, then build + `./pharmasea.exe -t` + one .e2e script
- [ ] Replace vendored `src/engine/frustum.h` (raylibExtras copy) with afterhours `plugins/frustum.h` (`make_frustum` + sphere/box tests) in rendering_system
- [ ] Pilot afterhours `InstanceBatcher`/`draw_model_instanced` (plugins/models.h) on most-repeated model; keep own ModelLibrary (glTF/fallback/lazy)
- [ ] Add afterhours `plugins/lod.h` hysteresis LOD alongside frustum culling
- [ ] Adopt afterhours `particles::Emitter` for drink/vomit particle effects (closes core-features particle item)
- [ ] Evaluate `RngStream` (save/load state) for map generation; keep pcg for network/replay-critical paths
- [x] E2E: adopt upstream results JSON (`write_results_json`/`--results-json`) + `testing::is_e2e_running()`, then typed `register_command`; keep hand parser in e2e_manager.h until then — done: hand parser deleted, scripts use `wait_until`, pending commands isolated in the singleton default collection (client map generation wipes its own collection)
- [ ] Use `files::TestRoots` in tests so they stop touching the real save folder; consider `files::get_cache_path()` for regenerable data
- [ ] Settings remap: use upstream `actions_using` conflict checks + `input::Repeat` for held repeat; enum `is_held/is_pressed` for new code
- [ ] Confirm fixed-tick wiring, then set `SystemManager::max_fixed_ticks` catch-up cap
- [ ] Opportunistic: upstream `src/image.h` (`image_size`/`decode_image`), `FrameRateStats`, `font_size_per_point`/`load_font_from_memory`
- [ ] Skip (recorded in report): imm UI suite, house_of_cards (revisit after instancing), lit shader, sound wrappers, toast plugin, query indexes, expect_drawn capture asserts

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

**CI/perf hygiene:**
- [x] Add `make e2e` target: rebuild if needed, run `--e2e-dir`, surface per-script PASS/FAIL + exit code (today e2e is manual-only) — `make e2e` (fast set) and `make e2e-slow`, one process per script via `tests/e2e/run_e2e.sh`
- [x] Tag slow scripts (`03`, `06`, `09`) so a fast subset (`01,02,04,11,12`) can run per-commit and the full dir nightly — `@slow`/`@xfail` header tags parsed; `run_e2e.sh` excludes both by default (`12` is `@xfail`, runs standalone)
- [ ] Frame-time smoke: reuse upstream `FrameRateStats` (afterhours onboarding) to fail `03_full_day_cycle` if avg frame time regresses past a generous threshold — catch perf cliffs, not frame-perfect timing — blocked on G-E5 (no perf provider in hand parser)

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
