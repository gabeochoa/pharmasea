# FSM audit — implicit state machines in `src/` (Pub Panic! / pharmasea)

Dispatch: `d-3bdd4ea7349e` · Base: `main` @ `8ebf7eeb` · Scope: `src/` only (vendor/afterhours excluded except as consumer notes) · Report-only: no `src/` code was changed for this audit.

Method: systematic greps for `enum|switch|state|mode|phase|stage|step|status|bool` across `src/` (72 enum declarations, 94 `switch` sites), then full-file reads of every candidate. Search hits only located candidates; every machine below was confirmed by reading its defining file and its mutation/dispatch sites. Candidates that are not machines are listed explicitly under “Keep-bool / not-a-machine” with reasons (stuck rule: unconfirmed candidates are classified, not stalled on).

Conventions observed (drive the recommended forms): C++20, `afterhours` ECS components as `struct : BaseComponent` POD-like data ([project_context.md](/Users/gabeochoa/p/pharmasea/project_context.md)), `PascalCase` types/enums, `enum struct`/`enum class` with `magic_enum` names, systems as `afterhours::System<...>` with `should_run()` gating, serialization via `zpp::bits` `serialize(archive, self)` field-order lists, network via GameNetworkingSockets + `network::ClientPacket` variant. Hard constraint applied throughout: **never renumber serialized or networked values** — `zpp::bits` archives are positional, `GameStateInfo`/`menu::State` travel on the wire ([src/network/types.h:64](/Users/gabeochoa/p/pharmasea/src/network/types.h:64)), replay files store raw `int` casts ([src/engine/simulated_input/input_replay.cpp:71](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/input_replay.cpp:71)), input-map persistence parses `menu::State` with `std::stoi` ([src/input_mapping_persistence.h:74](/Users/gabeochoa/p/pharmasea/src/input_mapping_persistence.h:74)).

UX = player-facing behaviour/feedback. DX = developer clarity, compiler exhaustiveness, one transition site, testability. AX = agent experience: named, greppable states/transitions with a single source of truth.

---

## 1. Summary table

| # | Machine | Location (definition) | Current encoding | #states | Priority | Explicit form | Effort |
|---|---------|------------------------|----------|---------|----------|---------------|--------|
| M01 | Game flow (`game::State`) | [src/engine/statemanager.h:122](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:122) | Plain `enum` + history stack, unconstrained `set()` | 6 (+Pause overlay) | P1 | Keep values; add validating `transition_to()` table as only mutator | M |
| M02 | Menu/navigation (`menu::State`) | [src/engine/statemanager.h:87](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:87) | `enum struct` + history stack + layered-input coupling | 6 (incl. synthetic `UI=99`) | P1 | Keep values; transition table + guard host-override edge | M |
| M03 | Day/night round phase | [src/components/has_day_night_timer.h:9](/Users/gabeochoa/p/pharmasea/src/components/has_day_night_timer.h:9) | `is_day` bool + `needs_to_process_change` bool + countdown floats | 3 (+transition flag) | **P0** | Add `Phase` enum alongside serialized bools; bools become derived shim | M |
| M04 | Customer/employee AI | [src/components/is_ai_controlled.h:9](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:9) | `enum class State:int` + `optional next_state` + `resume_state` + per-state components/tags | 9 | P1 | Already near-explicit: close bypass setters, one commit site, exhaustive switches | M |
| M05 | Drink-order lifecycle | [src/components/can_order_drink.h:12](/Users/gabeochoa/p/pharmasea/src/components/can_order_drink.h:12) | Plain `enum OrderState` + uncapped counters | 4 | P2 | Keep enum; single `transition_to()` + exhaustive consumers | S |
| M06 | Toilet lifecycle | [src/components/is_toilet.h:8](/Users/gabeochoa/p/pharmasea/src/components/is_toilet.h:8) | Plain `enum State` + `user=-1` + uses counter | 3 | P2 | `enum class` (same values) + guarded `start_use/end_use/reset` only | S |
| M07 | Trigger-area lifecycle | [src/components/is_trigger_area.h:17](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h:17) | `Type` enum (dispatch) + progress/cooldown floats + ECS tags + global delete-mode bool | 4 phases × 13 types | P1 | Add `Phase` enum alongside floats; delete-mode becomes per-room state | M |
| M08 | Progression choice cycle | [src/components/is_progression_manager.h:145](/Users/gabeochoa/p/pharmasea/src/components/is_progression_manager.h:145) | `upgrade_index` modulo counter + `collectedOptions` bool | 2–3 | P2 | `ProgressionPhase` enum; index derived, not stored as phase | S |
| M09 | Network role/session | [src/network/network.h:42](/Users/gabeochoa/p/pharmasea/src/network/network.h:42) | Bitflag `Role` (`None|Host|Client`) + `username_set`/`ip_set` bools + `id==0` sentinel | 4 (+impossible combos) | **P0** | `SessionState` enum; keep wire `Role` bits only at boundary | M |
| M10 | Steam connection state | [src/network/internal/client.cpp:142](/Users/gabeochoa/p/pharmasea/src/network/internal/client.cpp:142), [src/network/internal/server.h:303](/Users/gabeochoa/p/pharmasea/src/network/internal/server.h:303) | Vendor `m_eState` switches ×2 + `running` bools + Local-hub bools | ~6 (vendor) | **P0** | Own `ConnectionState` wrapper mapped 1:1; one handler per side | M |
| M11 | Save/load slots | [src/save_game/save_game.h:20](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h:20) | `exists` bool + `optional header` + `save_version`/`magic` + CLI globals | 5 | **P0** | `SlotState`/`LoadResult` enums; on-disk bytes untouched | M |
| M12 | Settings window + key capture | [src/layers/settingslayer.h:15](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.h:15) | `ActiveWindow` enum + popup `show` bool + `InputType` + debounce timer | 2 windows × 2 capture | P2 | Keep window enum; add `CaptureState` enum for popup | S |
| M13 | Layered input mode | [src/game.cpp:126](/Users/gabeochoa/p/pharmasea/src/game.cpp:126), [src/input_mapping_setup.h:91](/Users/gabeochoa/p/pharmasea/src/input_mapping_setup.h:91) | `menu::State` reused as input layer; `on_change` → `set_active_layer` | = M02 | P1 | Keep (correct reuse); document as view of M02, single sync site | S |
| M14 | Fishing minigame | [src/components/has_fishing_game.h:8](/Users/gabeochoa/p/pharmasea/src/components/has_fishing_game.h:8) | `started` bool + `score==-1` sentinel + `direction=±1` + countdown | 3 | P2 | `Phase` enum alongside; sentinel kept for wire compat | S |
| M15 | Work/progress (machines, furniture) | [src/components/has_work.h:10](/Users/gabeochoa/p/pharmasea/src/components/has_work.h:10) | `pct` float 0..1 + `more_to_do`/`reset_on_empty` bools + `WorkFn` lambdas | 3 | P2 | Derived `WorkPhase` accessor; float stays serialized truth | S |
| M16 | Squirter processing | [src/components/is_squirter.h:7](/Users/gabeochoa/p/pharmasea/src/components/is_squirter.h:7) | `working` bool (in fact write-only) + countdown + id refs (`-1`) | 3 | P2 | `Phase` enum; delete or honour `working` | S |
| M17 | Spawner cadence | [src/components/is_spawner.h:15](/Users/gabeochoa/p/pharmasea/src/components/is_spawner.h:15) | `num_spawned/max` counters + `countdown` + 2 bools | 3 derived | P3 | Derived `SpawnerState` accessor only | S |
| M18 | Container backfill + indexer | [src/components/is_item_container.h:5](/Users/gabeochoa/p/pharmasea/src/components/is_item_container.h:5), [src/components/indexer.h:8](/Users/gabeochoa/p/pharmasea/src/components/indexer.h:8) | `gens/max_gens(-1=∞)` + `uses_indexer` bool + `index/max/changed` | 3 derived | P2 | Named constants + derived state; no new serialized field v1 | S |
| M19 | Held-item attachment | [src/components/can_hold_item.h:18](/Users/gabeochoa/p/pharmasea/src/components/can_hold_item.h:18), [src/components/is_item.h:7](/Users/gabeochoa/p/pharmasea/src/components/is_item.h:7) | Dual bookkeeping: holder `EntityRef` ↔ item `held_by/held_by_id` (`Unknown`/`-1`) | 2 (+stale) | P2 | Keep `update()` choke point; add invariant check + derived `HeldState` | S |
| M20 | Pneumatic-pipe pairing | [src/components/is_pnumatic_pipe.h:7](/Users/gabeochoa/p/pharmasea/src/components/is_pnumatic_pipe.h:7) | `paired` ref sentinel + `recieving` bool + `item_id=-1` (latter two **not serialized**) | 3 | **P0** | `PairingState` enum + serialize-or-derive fix (flagged) | S |
| M21 | Tutorial / NUX steps | [src/components/is_nux_manager.h:10](/Users/gabeochoa/p/pharmasea/src/components/is_nux_manager.h:10) | `is_active` bool + `initialized` bool + `is_tutorial_active` bool + 4 lambdas; ordered only by creation | ~14 steps × 4 phases | P2 | `NuxPhase` enum + explicit ordered step table | M |
| M22 | Dev bypass automation | [src/components/bypass_automation_state.h:5](/Users/gabeochoa/p/pharmasea/src/components/bypass_automation_state.h:5) | 9 bools + `remaining_rounds` + timers (`-1` sentinels) | ~7 sequential | P3 | `BypassPhase` enum (dev-only, unserialized) | S |
| M23 | Intro sequence | [src/intro/intro_runner.h:9](/Users/gabeochoa/p/pharmasea/src/intro/intro_runner.h:9) | `vector<IntroScene>` + per-scene `elapsed`/`bool update()` + show-intro bools | N scenes × 2 | P3 | Keep pipeline; optional `IntroPhase` for logging only | S |
| M24 | Replay validation | [src/engine/simulated_input/input_replay.cpp:19](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/input_replay.cpp:19) | `active` bool + `idx` cursor + `Type` enum dispatch | 3 | P3 | `ReplayState` enum (dev-only) | S |

## 2. Counts by category

| Category | Machines | Notes |
|----------|----------|-------|
| Already-enum, unconstrained transitions | M01, M02, M05, M06, M13 (=M02 view) | Enum exists; any code can `set()` any value. Fix is transition validation, not a new enum. |
| Bool-cluster / sentinel-encoded (true implicit FSMs) | M03, M09, M14, M16, M20, M21, M22 | Impossible combos representable today (e.g. Host\|Client, fishing scored-but-not-started). |
| Counter/float-as-phase | M07 (progress), M08, M15, M17, M18, M23, M24 | Continuous values are correct storage; phase should be a derived named view. |
| External/vendor state mirrored implicitly | M10 (+ M09 session edge) | Steam `m_eState` must never be renumbered; wrap, do not replace. |
| Pipeline / derived (keep, do not over-engineer) | M13 view, M17 accessor, M23 | Listed so future audits do not re-flag them. |
| **Total machines** | **24 records (23 distinct; M13 is a view of M02)** | Plus keep-bool/not-a-machine table (§5). |
| By priority | **P0: 5** (M03, M09, M10, M11, M20) · **P1: 5** (M01, M02, M04, M07, M13) · **P2: 9** (M05, M06, M08, M12, M14, M15, M16, M18, M19, M21 — 10 listed, M21 counted here) · **P3: 4** (M17, M22, M23, M24) | P0 = save/network risk or impossible state reachable today. P1 = core game/round/menu flow. P2 = AI/item/UI local. P3 = trivial/dev-only. |
| By effort | S: 13 · M: 10 · L: 0 | No L: every conversion fits the add-alongside → route → exhaust → delete sequence without a big-bang rewrite. |

Priority recount (exact): P0 = M03, M09, M10, M11, M20 (5). P1 = M01, M02, M04, M07, M13 (5). P2 = M05, M06, M08, M12, M14, M15, M16, M18, M19, M21 (10). P3 = M17, M22, M23, M24 (4). Total 24 records.

## 3. Shared explicit-FSM pattern (codebase style)

Style basis: `IsAIControlled` already demonstrates the house pattern — `enum class State : int` with explicit `=0` ([src/components/is_ai_controlled.h:9](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:9)), staged `next_state` + single commit system ([src/system/ai/transition/ai_commit_next_state_system.h:14](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/ai_commit_next_state_system.h:14)), exhaustive `switch` with **no `default`** ([src/system/ai/transition/ai_setup_state_components_system.h:46](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/ai_setup_state_components_system.h:46)). Generalise it; do not invent a framework. Vendor `afterhours` stays untouched.

Rules for every conversion:

1. **Values are frozen.** New `enum class X : int` reuses the exact old numeric values (including gaps like `game::State::Paused = 4`, `ModelTest = 7`, `menu::State::UI = 99`). Appending new enumerators at the end is allowed; inserting/renumbering is not.
2. **One mutation site.** All writes go through a single validating `transition_to(next, reason)` (components) or the existing singleton `set()` narrowed by a transition table (M01/M02). Direct field writes are deleted last, after callers are routed.
3. **Exhaustive dispatch.** Consumers `switch` over the enum with no `default:` (compiler `-Wswitch` becomes the safety net); fallthrough must be commented as intentional (existing style: `case game::Paused: // fall through` in [src/system/core/system_manager.cpp:67](/Users/gabeochoa/p/pharmasea/src/system/core/system_manager.cpp:67)).
4. **Serialized truth moves last.** `zpp::bits` field order in each `serialize()` is wire/save format. Phase 1 adds the enum **alongside** (unserialized or appended at the end); only after save/load + network soak do the old bools get deleted. Appending a field changes the blob — treat as save-version bump (`save_version` in [src/save_game/save_game.h:26](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h:26)) and gate it.
5. **Behaviour-preserving first.** Any behaviour change (e.g. rejecting an invalid transition that today silently succeeds) is an opt-in second step, flagged per machine.

Sketch (component form, mirroring `IsAIControlled` + `HasDayNightTimer` naming):

```cpp
// In the component header — values frozen, do not renumber.
enum class Phase : int { Planning = 0, InRound = 1, Transitioning = 2 };
Phase phase = Phase::Planning;  // appended LAST in serialize() when enabled

// The ONLY mutation site. Returns false (and logs) on an illegal edge;
// callers that must not fail use transition_or_keep() during migration.
bool transition_to(Phase next) {
    const bool ok = (phase == Phase::Planning && next == Phase::Transitioning) ||
                    (phase == Phase::Transitioning && next == Phase::InRound) ||
                    (phase == Phase::InRound && next == Phase::Transitioning) ||
                    (phase == Phase::Transitioning && next == Phase::Planning);
    if (!ok) { log_warn("illegal phase {} -> {}", phase, next); return false; }
    phase = next;
    return true;
}
```

Safe migration sequence (every machine): (1) add enum + `transition_to()` alongside, old fields remain truth; (2) route all writers through it, add `log_warn` on illegal edges (no behaviour change yet); (3) convert readers to exhaustive switches; (4) flip truth to the enum, keep old bools as derived shims for one release; (5) delete shims + bump `save_version` if `serialize()` changed. Build must pass (`make`) after every step; never land (4)+(5) together.

---

## 4. Per-machine records

### M01 — Game flow (`game::State`) · P1 · M

- **Location:** enum [src/engine/statemanager.h:122](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:122) (`InMenu=0, Lobby=1, InGame=2, Paused=4, ModelTest=7, LoadSaveRoom=8` — gaps are load-bearing); manager [src/engine/statemanager.h:139](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:139); debug chooser [src/layers/debug_settings.h:9](/Users/gabeochoa/p/pharmasea/src/layers/debug_settings.h:9).
- **Current encoding:** plain `enum` (not `enum class`) in `StateManager2<game::State>` with an unbounded `std::stack` history (`set()` pushes, `go_back()` pops, warn at depth ≥10, [src/engine/statemanager.h:48](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:48)). `Paused` is modelled as a *state* whose return target lives only in `previous()`/`is_paused_in()` ([src/engine/statemanager.h:156](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:156)).
- **States:** `InMenu, Lobby, InGame, Paused(overlay over Lobby|InGame), ModelTest, LoadSaveRoom`.
- **Transitions + triggers:** `Lobby→InGame` (Play trigger, [src/system/trigger/trigger_cb_on_full_progress_system.h:178](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_cb_on_full_progress_system.h:178)); `Lobby→ModelTest`/`→LoadSaveRoom` and returns (same file :150–:195); `InGame→Lobby` via progression/store returns; `*→Paused→go_back()` ([src/layers/gamelayer.cpp:38](/Users/gabeochoa/p/pharmasea/src/layers/gamelayer.cpp:38), [src/layers/pauselayer.h:32](/Users/gabeochoa/p/pharmasea/src/layers/pauselayer.h:32)); host→client sync overwrites client state ([src/network/client.cpp:275](/Users/gabeochoa/p/pharmasea/src/network/client.cpp:275)); `reset()` on disconnect ([src/network/client.cpp:64](/Users/gabeochoa/p/pharmasea/src/network/client.cpp:64)).
- **Who mutates:** trigger system, pause/game layers, network client/server (`send_game_state_update`, [src/network/server.cpp:325](/Users/gabeochoa/p/pharmasea/src/network/server.cpp:325) area), debug layer. No single owner.
- **Implicitness problems:** any `set()` edge is legal (e.g. `InMenu→ModelTest` compiles); pause return depends on stack discipline — a stray `set()` while paused corrupts `go_back()`; `is_update_state()` excludes only `Paused|InMenu` so new states silently become “updatable” ([src/engine/statemanager.h:160](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:160)); server thread reads the singleton while main thread writes it (documented race, [src/engine/statemanager.h:14](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:14)).
- **Save/network risk:** **High if renumbered, low otherwise.** Values travel in `GameStateInfo.host_game_state` ([src/network/types.h:65](/Users/gabeochoa/p/pharmasea/src/network/types.h:65)) and replay files compare raw ints. Freeze values; do not “tidy” gaps.
- **UX/DX/AX decision:** UX — pause must stay an overlay with reliable resume (protect `go_back` semantics). DX — a transition table makes illegal edges loggable/testable without changing legal ones. AX — named `transition_to_game/lobby/model_test` already exist ([src/engine/statemanager.h:185](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:185)); extend that vocabulary instead of raw `set()`.
- **Recommended explicit form:** keep `game::State` values; add `bool GameState::transition_to(game::State)` with an explicit edge table as the *only* writer (keep `set()` as deprecated shim for one release); model pause as `PausedOver {Lobby, InGame}` captured at pause time rather than stack archaeology — flagged as optional step 2, behaviour-preserving step 1 keeps the stack.
- **Migration:** add table + route trigger/layer callers → log-only validation soak → exhaustive `should_run` switches → deprecate raw `set()`. Verify: `make`, `./pharmasea.exe --tests-only`, host+client pause/resume soak.

### M02 — Menu/navigation (`menu::State`) · P1 · M

- **Location:** enum [src/engine/statemanager.h:87](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:87) (`Root=0, About=1, Network=2, Settings=3, Game=4, UI=99`); singleton [src/engine/statemanager.h:105](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h:105).
- **Current encoding:** `enum struct` + same history-stack manager. Layers poll and early-return (`is_not(...)`) every frame — e.g. [src/layers/menulayer.h:30](/Users/gabeochoa/p/pharmasea/src/layers/menulayer.h:30), [src/layers/networklayer.cpp:49](/Users/gabeochoa/p/pharmasea/src/layers/networklayer.cpp:49).
- **States:** `Root, About, Network, Settings, Game`, plus `UI=99` — not a screen: an input-layer alias shared by Root/Settings/Network/About ([src/input_mapping_setup.h:82](/Users/gabeochoa/p/pharmasea/src/input_mapping_setup.h:82)).
- **Transitions + triggers:** Root→Network/Settings/About (menu buttons, [src/layers/menulayer.h:46](/Users/gabeochoa/p/pharmasea/src/layers/menulayer.h:46)); Network→Game+Lobby on connect ([src/layers/networklayer.cpp:232](/Users/gabeochoa/p/pharmasea/src/layers/networklayer.cpp:232)); Settings reachable from pause ([src/layers/pauselayer.h:77](/Users/gabeochoa/p/pharmasea/src/layers/pauselayer.h:77)); host forces client `Network→Game` only ([src/network/client.cpp:271](/Users/gabeochoa/p/pharmasea/src/network/client.cpp:271)).
- **Who mutates:** layers, network client, `reset_connections()` ([src/network/network.h:171](/Users/gabeochoa/p/pharmasea/src/network/network.h:171)), UI-test layer.
- **Implicitness problems:** README-known bug “client player cant change settings because `menu::State` is being overridden by host” (README TODO table) — the guard at client.cpp:271 is a one-edge patch on an otherwise unconstrained sync; `UI=99` conflates navigation with input layering (AX trap: searching `UI` finds no screen); history stack allows `go_back` into stale screens after network resets.
- **Save/network risk:** values persisted in input-mapping file via `std::stoi` ([src/input_mapping_persistence.h:74](/Users/gabeochoa/p/pharmasea/src/input_mapping_persistence.h:74)) and sent as `host_menu_state` ([src/network/types.h:64](/Users/gabeochoa/p/pharmasea/src/network/types.h:64)). Freeze `0..4, 99`.
- **UX/DX/AX decision:** UX — host must not yank a client out of Settings (fix the sync edge set, not just the one guarded edge). DX — screen set + input-layer set are different domains; splitting them removes the `99` hack. AX — greppable `Screen` vs `InputLayer` names.
- **Recommended explicit form:** step 1 (preserving): transition table on `MenuState` + explicit host-sync allowlist `{Network→Game}`. Step 2 (optional, flagged): split `UI` into `ProvidesLayeredInputMapping<InputLayer>` so `menu::State` holds screens only. Do not do step 2 in the same change as M13.
- **Migration:** table → route layers → soak with two clients (host in Settings, client in Settings) → optional split. Verify: `make`, tests-only, manual host/client settings soak.

### M03 — Day/night round phase (`HasDayNightTimer`) · P0 · M

- **Location:** [src/components/has_day_night_timer.h:9](/Users/gabeochoa/p/pharmasea/src/components/has_day_night_timer.h:9); sole phase-flipper [src/system/afterhours/gamelike/run_timer_system.h:15](/Users/gabeochoa/p/pharmasea/src/system/afterhours/gamelike/run_timer_system.h:15); consumers via [src/system/system_utilities.h:10](/Users/gabeochoa/p/pharmasea/src/system/system_utilities.h:10) and ~15 copied `should_run` bodies.
- **Current encoding:** `bool is_day = true` ([src/components/has_day_night_timer.h:80](/Users/gabeochoa/p/pharmasea/src/components/has_day_night_timer.h:80)) where `is_bar_open() == !is_day`; `bool needs_to_process_change` set by `open_bar()/close_bar()` (:39–:52) and cleared by a distant system ([src/system/afterhours/day_night_transition/reset_has_day_night_changed.h:29](/Users/gabeochoa/p/pharmasea/src/system/afterhours/day_night_transition/reset_has_day_night_changed.h:29)); phase length is `current_length` countdown, day identity is `day_count`/`days_until_rent_due` counters.
- **States (implicit):** `Planning` (bar closed/day, systems gated by `should_run_planning_system`), `InRound` (bar open/night), `Transitioning` (`needs_to_process_change == true`, most `should_run` return false). “Closing soon” is a fourth UX state derived ad hoc (`pct() <= 0.1f`, [src/system/ui/ui_rendering_system.cpp:227](/Users/gabeochoa/p/pharmasea/src/system/ui/ui_rendering_system.cpp:227)).
- **Transitions + triggers:** timer expiry `pass_time`→`is_round_over`→`close_bar()` (rent check/withdraw inside the transition!, run_timer_system.h `execute_close_bar`) or `open_bar()` (collects progression options first, `execute_open_bar`); transition systems (open doors, generate/clean store options, delete floating items, force-leave customers) run while `needs_to_process_change`, then the flag is reset — ordering is by system registration, not by an explicit state.
- **Who mutates:** `RunTimerSystem` only for `is_day`; a dozen systems *read* the flag pair to decide `should_run` (duplicated Sophie lookup + try/catch in each — e.g. [src/system/afterhours/inround/process_spawner_system.h:18](/Users/gabeochoa/p/pharmasea/src/system/afterhours/inround/process_spawner_system.h:18)).
- **Implicitness problems:** impossible combo `is_day` flipped but `needs_to_process_change` cleared early makes planning/inround systems run mid-transition (the exact bug class the per-system guards defend against); rent payment hidden inside a timer transition is undiscoverable (AX); `is_bar_open()` scans `oldAll` for the first timer entity ([src/system/core/system_manager.cpp:375](/Users/gabeochoa/p/pharmasea/src/system/core/system_manager.cpp:375)) so a second timer entity silently changes semantics.
- **Save/network risk:** **P0.** `serialize()` archives 7 fields positionally ending in `is_day` ([src/components/has_day_night_timer.h:84](/Users/gabeochoa/p/pharmasea/src/components/has_day_night_timer.h:84)); component replicates to clients every map sync. Any field insertion/reorder corrupts saves and desyncs clients.
- **UX/DX/AX decision:** UX — Planning vs InRound vs last-10% “closing” need distinct player feedback from one source. DX — one `phase()` replaces ~15 copied guards. AX — `Phase::Planning|InRound|Transitioning` is greppable; `is_day` is not.
- **Recommended explicit form:** add `enum class Phase : int { Planning=0, InRound=1, Transitioning=2 }` + `transition_to()` in `HasDayNightTimer`; `open_bar/close_bar` become thin wrappers; `is_day`/`needs_to_process_change` become derived shims. Append `phase` at the **end** of `serialize()` only with a `save_version` bump; until then keep it unserialized and reconstructed from the bools on load.
- **Migration:** derived `phase()` accessor (no storage) → route `should_run` helpers to it → add stored enum unserialized → flip + serialize-append + version bump. Verify each step: `make`, tests-only, full day cycle soak (plan→open→close→rent).

### M04 — Customer/employee AI (`IsAIControlled`) · P1 · M

- **Location:** [src/components/is_ai_controlled.h:9](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:9) (`Wander=0, QueueForRegister, AtRegisterWaitForDrink, Drinking, Bathroom, Pay, PlayJukebox, CleanVomit, Leave`); commit [src/system/ai/transition/ai_commit_next_state_system.h:28](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/ai_commit_next_state_system.h:28); component setup [src/system/ai/transition/ai_setup_state_components_system.h:46](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/ai_setup_state_components_system.h:46); request helper [src/system/ai/ai_shared_utilities.h:18](/Users/gabeochoa/p/pharmasea/src/system/ai/ai_shared_utilities.h:18).
- **Current encoding:** the most explicit machine in the codebase — authoritative `state`, staged `optional<State> next_state` (first-writer-wins, [src/components/is_ai_controlled.h:85](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:85)), `AITransitionPending`/`AINeedsResetting` tags, per-state payload components (`HasAIQueueState`, `HasAIDrinkState`, …) added/removed by an exhaustive switch with no `default`. Sub-state: `AIWaitInQueueState` (`has_set_position_before` bool + `queue_index=-1` sentinel, [src/components/ai_wait_in_queue_state.h:8](/Users/gabeochoa/p/pharmasea/src/components/ai_wait_in_queue_state.h:8)) and `CooldownInfo` timers (`initialized/enabled`, [src/components/cooldown_info.h:15](/Users/gabeochoa/p/pharmasea/src/components/cooldown_info.h:15)).
- **States/transitions:** Wander→QueueForRegister→AtRegisterWaitForDrink→Drinking→Pay→Leave (happy path); Wander↔{Bathroom (interrupt, return stored in `HasAIBathroomState.next_state`, ai_commit_next_state_system.h:45), PlayJukebox→Queue, CleanVomit (ability-gated)}; any→Leave forced at close ([src/system/ai/transition/ai_force_leave_commit_system.h:33](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/ai_force_leave_commit_system.h:33)); any (except Bathroom/Drinking/Leave)→Bathroom interrupt ([src/system/ai/transition/needs_bathroom_now_system.h:22](/Users/gabeochoa/p/pharmasea/src/system/ai/transition/needs_bathroom_now_system.h:22)).
- **Who mutates:** per-state systems via `request_next_state()`; **bypasses:** `set_state_immediately` (force-leave, commit itself), `set_initial_state` (makers, [src/entities/entity_makers.cpp:282](/Users/gabeochoa/p/pharmasea/src/entities/entity_makers.cpp:282)), direct `state` field is public.
- **Implicitness problems:** two return-address slots with different meanings — `resume_state` (wander-pause, [src/components/is_ai_controlled.h:57](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:57)) vs `HasAIBathroomState.next_state` — easy to confuse (AX); silent drop when `next_state` already pending (only `request_next_state(override_existing=true)` callers can preempt — intent invisible at call site); rendering switch duplicates the state list ([src/system/rendering/rendering_system.cpp:1038](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp:1038)) and will drift; `CanOrderDrink::OrderState` (M05) shadows Drinking/Ordering with no enforced coupling.
- **Save/network risk:** `serialize()` archives `state, next_state, abilities, resume_state` positionally ([src/components/is_ai_controlled.h:101](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:101)). Values `0..8` frozen. Appending a state at the end (=9) is safe; inserting is not.
- **UX/DX/AX decision:** UX — keep behaviour identical; explicit interrupt priorities (Bathroom vs Leave) improve stuck-customer bugs. DX — commit system is already the single site; finish the job. AX — this machine is the pattern donor for §3; naming its edge table helps agents most.
- **Recommended explicit form:** keep as-is structurally; (a) make `state` private with `state()` accessor, (b) add an `is_legal(from,to)` edge table checked in commit (log-only first), (c) rename/document the two return slots (`resume_after_wander` vs bathroom return), (d) drive rendering/setup from one `magic_enum::enum_for_each` list. No new enum needed.
- **Migration:** accessor shim → route bypasses through commit (except spawn init) → edge-table log soak → enforce. Verify: `make`, tests-only, replay specs ([src/tests/replay_specs.cpp](/Users/gabeochoa/p/pharmasea/src/tests/replay_specs.cpp)).

### M05 — Drink-order lifecycle (`CanOrderDrink::OrderState`) · P2 · S

- **Location:** [src/components/can_order_drink.h:12](/Users/gabeochoa/p/pharmasea/src/components/can_order_drink.h:12) (`NeedsReset, Ordering, DrinkingNow, DoneDrinking` — plain enum, implicit values 0..3).
- **Current encoding:** enum + counters (`num_orders_rem=-1` sentinel, bladder/tab/tip) that jointly gate transitions; only `current_order` + `order_state` are serialized ([src/components/can_order_drink.h:127](/Users/gabeochoa/p/pharmasea/src/components/can_order_drink.h:127)) — counters reset on load by design-or-accident (undocumented).
- **States/transitions:** NeedsReset→Ordering (`reset_customer`, :81); Ordering→DrinkingNow (implicit — set where? readers check `has_order()==Ordering`, :42; the DrinkingNow write is scattered in AI/drink flow); DrinkingNow→Ordering|DoneDrinking (`on_order_finished`, :51, decided by `num_orders_rem>0`).
- **Who mutates:** customer reset system, drink-serving flow, `on_order_finished` callers.
- **Implicitness problems:** duplicates M04 states (Ordering≈Queue/AtRegister, DrinkingNow≈Drinking) with no invariant linking them — desync presents as “customer waits forever / drinks forever”; `DrinkingNow` has no single writer to grep.
- **Save/network risk:** serialized pair is positional; freeze `0..3`. Note (finding, not FSM scope): unserialized counters mean a mid-drink save/load resurrects counts — flag for save owners.
- **UX/DX/AX decision:** DX/AX win is a single `transition_to(OrderState)` + one exhaustive consumer switch; UX unchanged.
- **Recommended explicit form:** `enum class` with same values, private field + `transition_to()`, document the M04↔M05 coupling invariant in one comment-free assertion helper (`order_matches_ai_state`).
- **Migration:** accessor → route writers → exhaustive readers. Verify: `make`, tests-only.

### M06 — Toilet lifecycle (`IsToilet`) · P2 · S

- **Location:** [src/components/is_toilet.h:8](/Users/gabeochoa/p/pharmasea/src/components/is_toilet.h:8) (`Available, InUse, NeedsCleaning`, plain enum); writers `start_use/end_use/reset` (:26–:40); render switch [src/system/rendering/rendering_system.cpp:1214](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp:1214); use check [src/system/core/sophie.cpp:73](/Users/gabeochoa/p/pharmasea/src/system/core/sophie.cpp:73).
- **States/transitions:** Available→InUse (`start_use(id)`); InUse→Available|NeedsCleaning (`end_use`, by `uses_remaining<=0`); NeedsCleaning→Available (`reset` after cleaning/overnight).
- **Who mutates:** bathroom AI system, cleaning work flow, day transition.
- **Implicitness problems:** `end_use()` callable from any state (double-decrement of `uses_remaining` possible); `occupied()==state!=Available` conflates InUse with NeedsCleaning for queue logic; `user` id is serialized? No — `serialize()` omits `user` ([src/components/is_toilet.h:45](/Users/gabeochoa/p/pharmasea/src/components/is_toilet.h:45)), so a loaded InUse toilet has `user=-1` (orphaned occupant).
- **Save/network risk:** `state, total_uses, uses_remaining` positional — freeze values `0,1,2`.
- **Recommended explicit form:** `enum class State : int` same values + guard in `end_use` (`if state!=InUse return`), keep `reset()` as the only NeedsCleaning exit. Small, behaviour-preserving except the (bug-fix, flagged) orphan case.
- **Migration:** enum class rename → guards → exhaustive render switch (already switch-based). Verify: `make`, tests-only.

### M07 — Trigger-area lifecycle (+ load/save delete-mode) · P1 · M

- **Location:** type enum [src/components/is_trigger_area.h:17](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h:17) (13 values incl. `Unset`); progress/cooldown floats :200s; dispatch [src/system/trigger/trigger_cb_on_full_progress_system.h:60](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_cb_on_full_progress_system.h:60); global `g_load_save_delete_mode` declared [src/system/trigger/trigger_cb_on_full_progress_system.h:25](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_cb_on_full_progress_system.h:25).
- **Current encoding:** two machines fused: (a) `Type` = *what fires* (exhaustive switch, good); (b) lifecycle = floats `completion_time_passed/max` + `cooldown_time_passed/max` + tags `TriggerAreaFullNeedsProcessing`/`GateTriggerWhileOccupied`/`TriggerFiredWhileOccupied`. Delete-mode is a process-global bool toggling tags on all load slots (:224).
- **States (lifecycle):** Idle→Filling (`should_wave()`, [src/components/is_trigger_area.h:56](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h:56))→Full (tag set)→Cooldown→Idle. Entrant policy is a separate 3-value enum `EntrantsRequired` (:37, already explicit — keep).
- **Who mutates:** entrant-count systems, percent system, callback system, dynamic-settings system ([src/system/trigger/update_dynamic_trigger_area_settings_system.h:36](/Users/gabeochoa/p/pharmasea/src/system/trigger/update_dynamic_trigger_area_settings_system.h:36)).
- **Implicitness problems:** phase exists only as float ratios + tag presence (component-presence-as-state); `should_progress()` runs validation as a side effect inside a `const` query with server/client divergence ([src/components/is_trigger_area.h:190](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h:190) approx — see `should_progress` in file); global delete-mode has no owner entity, invisible in saves.
- **Save/network risk:** `serialize()` archives 14 fields including `type` and both float pairs positionally ([src/components/is_trigger_area.h:230](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h:230) approx — see serialize in file). Freeze Type values; phase enum appended last only.
- **UX/DX/AX decision:** UX — fill/cooldown feedback already rendered from `progress()`; a named phase makes “why won’t it fire” diagnosable. DX — tags remain the scheduler, phase becomes the explanation. AX — `Phase` + `Type` separately greppable.
- **Recommended explicit form:** add `enum class Phase : int { Idle=0, Filling=1, Full=2, Cooldown=3 }` derived from floats/tags in v1 (unserialized), stored in v2; move `g_load_save_delete_mode` onto the Sophie/room entity as `LoadSaveMode { Load, Delete }`. Keep the Type switch untouched.
- **Migration:** derived phase accessor → render/debug use it → store it → per-room delete-mode. Verify: `make`, tests-only, lobby→load-save room soak incl. delete toggle.

### M08 — Progression choice cycle (`IsProgressionManager`) · P2 · S

- **Location:** [src/components/is_progression_manager.h:145](/Users/gabeochoa/p/pharmasea/src/components/is_progression_manager.h:145) (`upgrade_index`, `collectedOptions`); `upgrade_type()` = `upgrade_rounds[upgrade_index]`; `next_round()` cycles modulo; choice commit in trigger system :296–:347.
- **Current encoding:** counter-as-phase: index cycles `UpgradeType` sequence (`None/Upgrade/Drink`, `UpgradeType` in [src/dataclass/settings.h:147](/Users/gabeochoa/p/pharmasea/src/dataclass/settings.h:147)); `collectedOptions` bool gates option generation (`collect_progression_options` called from `execute_open_bar`).
- **States:** AwaitingOptions→OptionsReady (`collectedOptions=true`)→ChoiceMade→(next_round) AwaitingOptions with advanced index.
- **Implicitness problems:** index is both “round number mod N” and “what kind of choice” — adding an `UpgradeType` silently re-phases every future round; bool+index combos (collected with stale options) representable.
- **Save/network risk:** `upgrade_index` + `collectedOptions` serialized positionally among 10 fields — freeze; derive phase, don’t store it in v1.
- **Recommended explicit form:** derived `ProgressionPhase { AwaitingOptions, OptionsReady }` accessor + named `advance_cycle()` as only index writer. Keep storage.
- **Migration:** accessor → route writers → done. Verify: `make`, tests-only, two consecutive progression rounds.

### M09 — Network role/session (`network::Info`) · P0 · M

- **Location:** [src/network/network.h:42](/Users/gabeochoa/p/pharmasea/src/network/network.h:42) (`Role { s_None=1, s_Host=2, s_Client=4 }` bitflags, `desired_role`); `set_role` switch :74; username mixin :102; ip flag [src/network/client.h:20](/Users/gabeochoa/p/pharmasea/src/network/client.h:20); join id sentinel `Client::id = 0` ([src/network/client.h:28](/Users/gabeochoa/p/pharmasea/src/network/client.h:28) approx).
- **Current encoding:** bitmask role + independent bools (`username_set`, `ip_set`) + `id==0` means unjoined + `client_p->is_connected()` — at least 5 bools/flags jointly encode session state. Host internally also constructs a Client (loopback), so “host” = server+client pair.
- **States (implicit):** NoRole→(Host|Client chosen)→IpLocked→Connected→Joined(id>0)→Disconnected(reset). 
- **Implicitness problems:** **impossible states today:** `desired_role = s_Host|s_Client` passes both `is_host()` and `is_client()` (:52); `username_set`/`ip_set` can be true with `s_None`; disconnect is detected by polling `is_not_connected()` every tick and immediately `reset()`s both state managers ([src/network/client.cpp:60](/Users/gabeochoa/p/pharmasea/src/network/client.cpp:60) approx) — no distinct “reconnecting/lost” UX state; `set_role` failure mode is `log_warn` + half-initialised `client` (thread id still stamped, :97).
- **Save/network risk:** **P0 (session, not save).** `Role` bits and `MsgType` values ([src/network/types.h:27](/Users/gabeochoa/p/pharmasea/src/network/types.h:27)) are wire-visible via behaviour (who hosts, packet routing); `hashed_version` join check depends on stable enums. Do not renumber `MsgType` or `Role` bits.
- **UX/DX/AX decision:** UX — an explicit `ConnectionLost` state enables a reconnect prompt instead of silent menu reset. DX — one `SessionState` kills flag-combo bugs. AX — `session_state()` greppable vs five flags.
- **Recommended explicit form:** `enum class SessionState : int { NoRole=0, Hosting=1, Joining=2, InGame=3, ConnectionLost=4 }` owned by `network::Info`, `transition_to()` as only writer; keep `desired_role` bits as a wire-compat shim derived from it. Second (flagged, optional): surface ConnectionLost to UI instead of instant reset.
- **Migration:** derived `session_state()` → route `set_role/tick` → store enum → optional UX step. Verify: `make`, tests-only, host/join/leave/kill-host soak incl. LOCAL_ONLY mode.

### M10 — Steam connection state (GNS + Local hub) · P0 · M

- **Location:** client callback switch [src/network/internal/client.cpp:142](/Users/gabeochoa/p/pharmasea/src/network/internal/client.cpp:142); server callback switch [src/network/internal/server.h:303](/Users/gabeochoa/p/pharmasea/src/network/internal/server.h:303); interfaces [src/network/internal/client.h:45](/Users/gabeochoa/p/pharmasea/src/network/internal/client.h:45), [src/network/internal/server.h:35](/Users/gabeochoa/p/pharmasea/src/network/internal/server.h:35); local hub flags [src/network/internal/local_hub.h:25](/Users/gabeochoa/p/pharmasea/src/network/internal/local_hub.h:25) (`server_running`, per-endpoint `disconnected`).
- **Current encoding:** vendor `ESteamNetworkingConnectionState` (`m_eState`) switched in two places + `running`/`is_connected()` (handle-validity) bools + a parallel hand-rolled Local implementation with different signals. No shared vocabulary between GNS and Local paths.
- **States:** vendor Connecting/Connected/ClosedByPeer/ProblemDetectedLocally/Dead (exact set owned by Steam headers — do not enumerate from memory in code; map exhaustively at build time).
- **Implicitness problems:** the two switches can drift (one handles a state the other ignores — silent hang vs reset divergence with M09); `is_connected()==handle!=Invalid` is true before Connected; Local `disconnected` bool has no Connecting equivalent.
- **Save/network risk:** **P0.** Vendor enum values must never be renumbered or stored; map them at the boundary only.
- **Recommended explicit form:** own `enum class ConnectionState : int { Idle=0, Connecting=1, Connected=2, Closed=3, Failed=4 }` + `to_connection_state(ESteamNetworkingConnectionState)` (exhaustive, no default) used by both callbacks and by LocalClient/LocalServer. M09 consumes this; it does not read Steam types.
- **Migration:** mapper + logging in both callbacks → Local emits same states → M09 routes on it. Verify: `make`, GNS + LOCAL_ONLY join/kill soak.

### M11 — Save/load slots · P0 · M

- **Location:** [src/save_game/save_game.h:20](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h:20) (header `save_version=1`, `magic="PHARMSAVE"`, `hashed_build_version`); `SlotInfo { exists bool, optional header }` :95; manager `save_slot/load_slot/read_header/enumerate_slots`; fixers [src/save_file_load_fixers.cpp:38](/Users/gabeochoa/p/pharmasea/src/save_file_load_fixers.cpp:38); post-load fixups [src/post_deserialize_fixups.cpp](/Users/gabeochoa/p/pharmasea/src/post_deserialize_fixups.cpp); CLI path via `LOAD_SAVE_ENABLED/TARGET` globals ([src/globals.h:44](/Users/gabeochoa/p/pharmasea/src/globals.h:44) approx) and server one-shot `cli_load_applied` ([src/network/server.cpp:348](/Users/gabeochoa/p/pharmasea/src/network/server.cpp:348) approx).
- **Current encoding:** file existence + optional header + version ints jointly encode slot state; load success is a bare `bool` (all failures identical to callers — see trigger handlers returning early on `!ok`).
- **States (implicit):** Empty, PresentValid, PresentCorrupt (bad magic/truncated), VersionMismatch (save_version/build hash), Loaded (world replaced, players moved to InGame/Planning).
- **Implicitness problems:** corrupt vs version-mismatch vs missing all collapse to `false`/empty optional — UX shows nothing, AX has nothing to grep; `read_header` fast path vs full `load_file` can disagree; delete-mode (M07) deletes by slot number derived from `HasSubtype.type_index` with clamping (`<1→1`) hiding bad data.
- **Save/network risk:** **P0 by definition.** On-disk `zpp::bits` order (magic first, then split archive calls, save_game.h:45) is frozen. Any FSM work must be in-memory only.
- **UX/DX/AX decision:** UX — distinct “corrupt” vs “wrong version” slot labels. DX — `LoadResult` enum makes fixer/fixup ordering testable. AX — named results greppable in logs.
- **Recommended explicit form:** `enum class SlotState : int { Empty=0, Valid=1, Corrupt=2, VersionMismatch=3 }` from `enumerate_slots` + `enum class LoadResult : int { Ok=0, Empty=1, Corrupt=2, VersionMismatch=3, IoError=4 }`; no on-disk change. Optional (flagged): surface SlotState in load-room labels.
- **Migration:** result enums alongside bools → route trigger/CLI callers → UI labels (optional). Verify: `make`, tests-only ([src/tests/test_entity_serialization.h](/Users/gabeochoa/p/pharmasea/src/tests/test_entity_serialization.h)), save→load→save round-trip + truncated-file probe.

### M12 — Settings window + key-capture · P2 · S

- **Location:** [src/layers/settingslayer.h:15](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.h:15) (`ActiveWindow { Root=0, KeyBindings=1 }`); popup `{ show bool, menu::State state, InputName input }` :20; dispatch switches [src/layers/settingslayer.cpp:91](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.cpp:91), :339; capture lambdas :362–:434; `selected_input_type : InputType` ([src/engine/keymap.h:22](/Users/gabeochoa/p/pharmasea/src/engine/keymap.h:22)).
- **States:** Root ↔ KeyBindings; capture sub-state Idle→Capturing (`show==true`, next key/button binds) with device filter `selected_input_type` (Keyboard/Gamepad switches :109/:383/:422).
- **Implicitness problems:** popup target (`state`+`input`) meaningful only when `show` — stale target binds to wrong action if shown without reset; capture has no Cancel state distinct from “bind Escape”; dropdown bool `windowSizeDropdownState` is a second ad-hoc popup encoding.
- **Save/network risk:** low — keymap *file* stores `menu::State` by int (`stoi`, [src/input_mapping_persistence.h:74](/Users/gabeochoa/p/pharmasea/src/input_mapping_persistence.h:74)); `ActiveWindow` itself is runtime-only. Freeze InputName/InputType values (keymap file + replay).
- **Recommended explicit form:** keep `ActiveWindow`; add `enum class CaptureState : int { Idle=0, Capturing=1 }` replacing `show`; popup fields valid only in Capturing (assert in debug). 
- **Migration:** enum alongside bool → route show/hide → delete bool. Verify: `make`, rebind + cancel soak (keyboard + gamepad).

### M13 — Layered input mode (view of M02) · P1 · S

- **Location:** registration [src/game.cpp:126](/Users/gabeochoa/p/pharmasea/src/game.cpp:126); sync callback [src/game.cpp:132](/Users/gabeochoa/p/pharmasea/src/game.cpp:132) (`MenuState on_change → ProvidesLayeredInputMapping::set_active_layer`); maps [src/input_mapping_setup.h:18](/Users/gabeochoa/p/pharmasea/src/input_mapping_setup.h:18); per-frame dispatch switches [src/system/input/input_process_manager.cpp:182](/Users/gabeochoa/p/pharmasea/src/system/input/input_process_manager.cpp:182), :878, :902.
- **Assessment:** correctly reuses `menu::State` as the layer key — this is a *view*, not a second machine. Risks are coupling risks: any M02 transition-table change must keep the callback as the single sync site; `MAP_VIEWER` startup deliberately skips the callback ([src/game.cpp:296](/Users/gabeochoa/p/pharmasea/src/game.cpp:296)) — document as intentional Idle layer.
- **Recommended form:** no new enum. Add a one-line contract comment-free guard: `set_active_layer` called from exactly one site (grep-enforced in review). If M02 step-2 split happens, this becomes `InputLayer` with a `to_input_layer(Screen)` mapper.
- **Migration:** none standalone; ride M02. Verify with M02 soak (menu arrows vs in-game keys).

### M14 — Fishing minigame (`HasFishingGame`, champagne) · P2 · S

- **Location:** [src/components/has_fishing_game.h:8](/Users/gabeochoa/p/pharmasea/src/components/has_fishing_game.h:8); tick [src/system/input/inround/pass_time_for_active_fishing_games_system.h:16](/Users/gabeochoa/p/pharmasea/src/system/input/inround/pass_time_for_active_fishing_games_system.h:16) (`pass_time` :52, `go` :78).
- **Current encoding:** `started` bool + `score=-1` sentinel (`has_score()==score!=-1`) + `direction=±1` + `countdown`. Impossible combos: scored while `!started`; `go()` after scored is a no-op by sentinel, not by state.
- **States:** NotStarted→Running (`go`)→Scored (countdown expiry, band lookup by `score_band`).
- **Save/network risk:** `serialize()` archives 7 fields including `started` positionally — freeze order; phase appended last only.
- **Recommended explicit form:** `enum class Phase : int { NotStarted=0, Running=1, Scored=2 }` derived in v1 (`Scored` iff `has_score()`), stored in v2; keep `score=-1` wire shim. UX: progress bar already keyed on `show_progress_bar()==started` (:46) — phase makes “done” feedback explicit.
- **Migration:** derived accessor → readers → stored enum. Verify: `make`, tests-only, open-champagne soak.

### M15 — Work/progress (`HasWork`) · P2 · S

- **Location:** [src/components/has_work.h:10](/Users/gabeochoa/p/pharmasea/src/components/has_work.h:10); reset consumer [src/system/input/inround/reset_empty_work_furniture_system.h:15](/Users/gabeochoa/p/pharmasea/src/system/input/inround/reset_empty_work_furniture_system.h:15).
- **Current encoding:** `pct_work_complete` float + `more_to_do` bool (default true “forces has-more to run”) + `reset_on_empty`/`hide_*` bools + per-entity `WorkFn` lambda doing the real transitions. Only `hide_progress_bar_on_full` + `pct` are serialized (:80) — `more_to_do` deliberately(?) unserialized.
- **States (derived):** Idle (`pct==0`), Working (`0<pct<1`), Complete (`pct>=1`, `is_work_complete()`), with `more_to_do` as “repeatable” orthogonal flag.
- **Implicitness problems:** completion is a float comparison repeated at call sites; WorkFn lambdas mutate pct freely (no single writer); progress-bar visibility logic (`can_show_progress_bar`, :22) re-derives phase a third way.
- **Recommended explicit form:** derived `WorkPhase { Idle, Working, Complete }` accessor + `complete()`/`reset()` as preferred writers. Do **not** store a phase (float remains truth; avoids save change).
- **Migration:** accessor → bar/render readers → WorkFn writers. Verify: `make`, tests-only, fill/drain a machine in-game.

### M16 — Squirter (`IsSquirter`) · P2 · S

- **Location:** [src/components/is_squirter.h:7](/Users/gabeochoa/p/pharmasea/src/components/is_squirter.h:7) (`pass_time` :22, `reset` :15); driver [src/system/sixtyfps/process_squirter_system.h:20](/Users/gabeochoa/p/pharmasea/src/system/sixtyfps/process_squirter_system.h:20).
- **Current encoding:** countdown `sq_time` + `working` bool that is written by `reset()`/read by `is_working()` but **never set true** in the read driver — phase is actually encoded by held-ref ids (`item_id()==-1`, `drink_id()`). Finding: `working` is dead-or-missing logic; confirm intent before deleting (list, don’t guess-fix in FSM pass).
- **States:** Idle (no held drink)→Armed (item matched)→Pouring (countdown)→Done (`set_drink_id` + reset).
- **Save/network risk:** `working` + times serialized positionally — keep field, derive phase from ids+timer.
- **Recommended explicit form:** derived `Phase { Idle, Armed, Pouring }`; resolve `working` (wire it in `update()` or delete with save-version note). 
- **Migration:** accessor → driver switch → working resolution (separate flagged fix). Verify: `make`, soda/alcohol squirt soak.

### M17 — Spawner cadence (`IsSpawner`) · P3 · S (accessor only)

- **Location:** [src/components/is_spawner.h:15](/Users/gabeochoa/p/pharmasea/src/components/is_spawner.h:15); driver [src/system/afterhours/inround/process_spawner_system.h:28](/Users/gabeochoa/p/pharmasea/src/system/afterhours/inround/process_spawner_system.h:28). Counters `num_spawned/max_spawned` (`max=0` default means immediately `hit_max`), `countdown/spread`, bools `showsProgressBar/prevent_duplicate_spawns` (config, not phase).
- **Derived states:** Exhausted (`hit_max`) / Ready (`countdown<=0`) / Cooling. Recommend **accessor only** (`state()`), no stored enum, no serialize change (4 fields positional). Trivial; do last.

### M18 — Container backfill + indexer · P2 · S

- **Location:** [src/components/is_item_container.h:25](/Users/gabeochoa/p/pharmasea/src/components/is_item_container.h:25) (`gens/max_gens`, `max_gens<0` = unlimited in `hit_max`); [src/components/indexer.h:11](/Users/gabeochoa/p/pharmasea/src/components/indexer.h:11) (`index/max_value/changed`; `max==0|1` = frozen in `increment`); backfill [src/system/core/system_manager.cpp:170](/Users/gabeochoa/p/pharmasea/src/system/core/system_manager.cpp:170) approx (`process_is_container_and_should_backfill_item` + `fix_container_item_type` switch :141).
- **Assessment:** counters are correct storage; implicit parts are sentinels (`-1`, `0`) and `changed` render-dirty flag. Recommend named constants (`kUnlimitedGenerations=-1`) + derived `ContainerState { Empty, Stocked, Exhausted }` for debug/render. `serialize()` stores only `gens/max_gens` (container type re-derived on load) — keep.

### M19 — Held-item attachment (`CanHoldItem` ↔ `IsItem`) · P2 · S

- **Location:** holder [src/components/can_hold_item.h:29](/Users/gabeochoa/p/pharmasea/src/components/can_hold_item.h:29) (`update` overloads — already a choke point); item side [src/components/is_item.h:9](/Users/gabeochoa/p/pharmasea/src/components/is_item.h:9) (`held_by=Unknown`, `held_by_id=-1`, `is_held()` derived); held-position dispatch [src/system/sixtyfps/afterhours_held_position_helpers.h:20](/Users/gabeochoa/p/pharmasea/src/system/sixtyfps/afterhours_held_position_helpers.h:20) (`Positioner` enum [src/components/custom_item_position.h:11](/Users/gabeochoa/p/pharmasea/src/components/custom_item_position.h:11), itself unserialized by design).
- **Implicitness problems:** dual bookkeeping can desync (holder ref cleared without item `held_by` reset — `update(nullptr)` :56 does not touch the old item); stale refs treated as empty (:22) masking leaks; furniture-holding variants (`CanHoldFurniture`/`CanHoldHandtruck` tag-style components) parallel the same idea without shared vocabulary.
- **Save/network risk:** holder serializes `held_by, held_item, last_held, filter`; item side serializes **nothing** ([src/components/is_item.h:50](/Users/gabeochoa/p/pharmasea/src/components/is_item.h:50) approx) — held-by is reconstructed on load. Any change must preserve that asymmetry or bump save version.
- **Recommended explicit form:** keep `update()` as sole writer; add debug-only invariant check (holder↔item back-pointer agreement) + derived `HeldState { Empty, Holding }`. No stored enum.

### M20 — Pneumatic-pipe pairing (`IsPnumaticPipe`) · P0 · S

- **Location:** [src/components/is_pnumatic_pipe.h:7](/Users/gabeochoa/p/pharmasea/src/components/is_pnumatic_pipe.h:7); pairing [src/system/afterhours/gamelike/process_pnumatic_pipe_pairing_system.h:12](/Users/gabeochoa/p/pharmasea/src/system/afterhours/gamelike/process_pnumatic_pipe_pairing_system.h:12); movement [src/system/afterhours/inround/process_pnumatic_pipe_movement_system.h:19](/Users/gabeochoa/p/pharmasea/src/system/afterhours/inround/process_pnumatic_pipe_movement_system.h:19).
- **Current encoding:** `paired` EntityRef (invalid = unpaired) + `bool recieving` [sic] + `int item_id=-1`. **`serialize()` archives only `paired`** — `recieving`/`item_id` are lost on save/load and host→client sync (in-flight item vanishes / pipe stuck). That data-loss edge is why this small machine is P0.
- **States:** Unpaired→PairedIdle→Transferring(`recieving`)→PairedIdle.
- **Recommended explicit form:** derived `PairingState` now; separately flagged fix: either serialize `recieving,item_id` (append at end + `save_version` bump) or derive them from world state on load. Do not silently pick — needs owner confirmation (see §6 open questions).
- **Migration:** accessor → pairing/movement switches → serialization decision. Verify: `make`, pipe transfer save-during-transfer probe.

### M21 — Tutorial / NUX steps (`IsNux`/`IsNuxManager`) · P2 · M

- **Location:** [src/components/is_nux_manager.h:10](/Users/gabeochoa/p/pharmasea/src/components/is_nux_manager.h:10) (`IsNux { is_active, time_shown, 4 lambdas }`, `IsNuxManager { initialized }`); driver [src/system/sixtyfps/process_nux_updates_system.h:16](/Users/gabeochoa/p/pharmasea/src/system/sixtyfps/process_nux_updates_system.h:16) (creation-ordered steps, “there should only ever be one” active by `gen_first`); gate `is_tutorial_active` ([src/components/is_round_settings_manager.h:14](/Users/gabeochoa/p/pharmasea/src/components/is_round_settings_manager.h:14)).
- **States per step:** Dormant→Eligible (`shouldTrigger`)→Active (`is_active`, `onTrigger` fired)→Complete (`isComplete` or parent died → `cleanup=true`). Sequence order is implicit in `_create_nuxes` creation order + query order — not data.
- **Implicitness problems:** at-most-one-active invariant enforced only by query; eligibility lambdas capture ids that may be stale; `initialized` + tutorial-off + lobby-exit triple gate scattered. System header itself notes “currently broken” (process_nux_updates_system.h:13 approx).
- **Save/network risk:** `is_active, entityID, content, ghost` serialized; lambdas not — phase enum appended last only.
- **UX/DX/AX decision:** UX — named current step enables progress hints/debug skip. DX — ordered `Step` table replaces creation-order coupling. AX — `NuxPhase` + step names greppable.
- **Recommended explicit form:** `enum class NuxPhase : int { Dormant=0, Eligible=1, Active=2, Complete=3 }` + `enum class TutorialStep` in creation order; keep lambdas as step payload. Behaviour-preserving (same eligibility/completion).
- **Migration:** phase derived from `is_active`+lambdas → driver switch → step table. Verify: `make`, tutorial playthrough (register→FFD→customer→serve).

### M22 — Dev bypass automation (`BypassAutomationState`) · P3 · S (dev-only)

- **Location:** [src/components/bypass_automation_state.h:5](/Users/gabeochoa/p/pharmasea/src/components/bypass_automation_state.h:5) (unserialized component: `configured/remaining_rounds`, `exit_on_complete/recording/bypass/initialized/completed` + per-flow `play_clicked/username_saved/host_clicked/start_clicked/lobby_w_held/ingame_w_held` + `-1` time sentinels); globals [src/globals.h:34](/Users/gabeochoa/p/pharmasea/src/globals.h:34) (`BYPASS_MENU/BYPASS_ROUNDS/EXIT_ON_BYPASS_COMPLETE`).
- **Assessment:** a genuine sequential flow (Startup→Menu→Lobby→InGame→RoundComplete×N→Exit) encoded as independent bools — invalid combos harmless (dev harness) but undebuggable. Recommend `enum class BypassPhase` + single `advance()`; no save/network impact. Replay active/cursor (M24) is the sibling dev machine in [src/engine/simulated_input/input_replay.cpp:32](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/input_replay.cpp:32).

### M23 — Intro sequence (`IntroRunner`) · P3 · S (keep pipeline)

- **Location:** [src/intro/intro_runner.h:9](/Users/gabeochoa/p/pharmasea/src/intro/intro_runner.h:9) (`vector<unique_ptr<IntroScene>>`, `update()->bool` done); scenes [src/intro/logo_intro_scene.h](/Users/gabeochoa/p/pharmasea/src/intro/logo_intro_scene.h), [src/intro/primary_intro_scene.h](/Users/gabeochoa/p/pharmasea/src/intro/primary_intro_scene.h) (`elapsed`, `holdAfterComplete`, `statusText`); show flags computed in `startup()` ([src/game.cpp:196](/Users/gabeochoa/p/pharmasea/src/game.cpp:196)).
- **Assessment:** linear pipeline, not a branching machine. Keep; optionally add `IntroPhase { Logo, Raylib, Loading, Done }` name for logs only. Do not build a framework for 3 scenes.

### M24 — Replay validation (`input_replay`) · P3 · S (dev-only)

- **Location:** [src/engine/simulated_input/input_replay.cpp:19](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/input_replay.cpp:19) (`ReplayEvent::Type` enum — explicit, keep; `ReplayState { active bool, idx cursor }` :32).
- **Assessment:** Loading→Playing→Done(encoded by `active=false` at exhaustion, :281) + validation side-channel ([src/engine/simulated_input/replay_validation.cpp:13](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/replay_validation.cpp:13)). Recommend `enum class ReplayPhase` for log clarity; recorded file format (int state casts) frozen regardless.

---

## 5. Keep-bool / not-a-machine (confirmed by full read — do not convert)

| Candidate | Location | Verdict | Reason |
|-----------|----------|---------|--------|
| End-of-round blockers | [src/components/collects_customer_feedback.h:39](/Users/gabeochoa/p/pharmasea/src/components/collects_customer_feedback.h:39) (`WaitingReason` bitset) | **Keep as flags** | Reasons are simultaneous, not exclusive (customers-in-store + bar-dirty at once). An exclusive enum would *lose* information. `text_reason` switch (:60) is display dispatch, not a machine. |
| AI abilities | [src/components/is_ai_controlled.h:29](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h:29) (`AbilityFlags` bitmask) | Keep flags | Capabilities compose; correctly a bitmask. |
| Generic timer primitive | [src/components/cooldown_info.h:15](/Users/gabeochoa/p/pharmasea/src/components/cooldown_info.h:15) (`initialized/enabled/remaining`) | Keep | `initialized=false` + `remaining` is a timer, not a lifecycle; shared by AI substates. Its TODO (split gating vs duration naming) is a naming task, not FSM. |
| Patience drain toggle | [src/components/has_patience.h:20](/Users/gabeochoa/p/pharmasea/src/components/has_patience.h:20) (`should_subtract`, unserialized) | Keep-bool (flag finding) | Genuine 2-state drain on/off. Note: unserialized means pause-state resets on load — behaviour note for save owners, not an FSM conversion. |
| Speech bubble / name / ghost tags | [src/components/has_speech_bubble.h:10](/Users/gabeochoa/p/pharmasea/src/components/has_speech_bubble.h:10), [src/components/bool_state.h:13](/Users/gabeochoa/p/pharmasea/src/components/bool_state.h:13) (`BoolState<Tag>` family: `CanBeHeld`, `CanBeGhostPlayer`, …) | Keep-bool | Single-bit presence. Codebase convention already prefers tag components over bools; migrating `BoolState` → pure tags is a separate cleanup, not FSM work. |
| Conveyor position | [src/components/conveys_held_item.h:11](/Users/gabeochoa/p/pharmasea/src/components/conveys_held_item.h:11) (`relative_item_pos` −0.5..0.5) | Not a machine | Continuous interpolation; endpoints are constants, no discrete transitions. |
| Toast fade | [src/engine/toastmanager.h:15](/Users/gabeochoa/p/pharmasea/src/engine/toastmanager.h:15) (`timeHasShown/timeToShow`, `pctOpen` derived) | Not a machine | Continuous lifetime; expiry is collection cleanup. `AnnouncementType` switch in [src/layers/toastlayer.h:34](/Users/gabeochoa/p/pharmasea/src/layers/toastlayer.h:34) is colour mapping. |
| Camera | [src/camera.h:8](/Users/gabeochoa/p/pharmasea/src/camera.h:8) (`GameCam` angles/distances; cursor hide/show edge in [src/layers/gamelayer.cpp:41](/Users/gabeochoa/p/pharmasea/src/layers/gamelayer.cpp:41)) | Not a machine | Continuous orbit control; the `was_left_down` static is edge detection, not a mode. No discrete camera modes exist to name. |
| Map generation | [src/map_generation/pipeline.h:10](/Users/gabeochoa/p/pharmasea/src/map_generation/pipeline.h:10) (`LayoutSource`, `BarArchetype`, bounded retry + WFC→Simple fallback in `pipeline.cpp`) | Keep (stateless pipeline) | Pure function with attempt loop; no runtime state persists between calls. `FailureClass/FailureKind` ([src/map_generation/playability_spec.h:22](/Users/gabeochoa/p/pharmasea/src/map_generation/playability_spec.h:22)) are result taxonomies. |
| Data/tag dispatch enums | `EntityType`, `Drink`/`Ingredient`, `IsTriggerArea::Type` dispatch half, `IsFloorMarker::Type`, `UpgradeType/UpgradeClass`, `ClientPacket::MsgType`, `DynamicType`, `Positioner`, building/door enums | Not machines | Identity/classification with switch-based behaviour selection. Exhaustive-switch hygiene applies (no `default` where feasible), but there are no transitions to validate. `MsgType` values are wire format — frozen. |
| Own-UI widget states | [src/engine/ui/state.h:44](/Users/gabeochoa/p/pharmasea/src/engine/ui/state.h:44) (`Toggle/Dropdown/Checkbox/ButtonList/Slider/Textfield` + `State<T>::changed_since`), `windowSizeDropdownState` bool | Keep (P3, out of FSM scope) | Per-widget interaction state owned by the immediate-mode UI; `focused/selected` are indices, `on` is a value. Converting to a shared FSM would fight the immediate-mode model. Vendor afterhours widget state explicitly excluded. |
| Settings/content data | `ConfigKey/Operation/UpgradeType`, `CanChangeSettingsInteractively::Style` (2 values incl. `Unknown`) | Not machines (Style: keep) | Config keys/operations are data; `Style` is a 1-real-value tag — add values as features land, no machine needed. |
| Thread role | [src/engine/thread_role.h:18](/Users/gabeochoa/p/pharmasea/src/engine/thread_role.h:18) (`Role { ClientMain, Server }` thread-local) | Keep | Already an explicit 2-value enum with single setter; cited by M01/M10 as context, not a conversion target. |
| App/layer presence | [src/engine/app.h:50](/Users/gabeochoa/p/pharmasea/src/engine/app.h:50) (`running` bool, layer stack) | Keep-bool | Process lifetime flag; layers gate on M01/M02 states rather than owning state. |

Key file links for the data-dispatch row: [src/entities/entity_type.h:9](/Users/gabeochoa/p/pharmasea/src/entities/entity_type.h:9) (`EntityType`), [src/dataclass/ingredient.h:9](/Users/gabeochoa/p/pharmasea/src/dataclass/ingredient.h:9) (`Drink`/`Ingredient`), [src/components/is_floor_marker.h:10](/Users/gabeochoa/p/pharmasea/src/components/is_floor_marker.h:10) (`IsFloorMarker::Type`), [src/network/types.h:27](/Users/gabeochoa/p/pharmasea/src/network/types.h:27) (`MsgType`), [src/components/has_dynamic_model_name.h:14](/Users/gabeochoa/p/pharmasea/src/components/has_dynamic_model_name.h:14) (`DynamicType`), [src/components/can_change_settings_interactively.h:7](/Users/gabeochoa/p/pharmasea/src/components/can_change_settings_interactively.h:7) (`Style`), [src/dataclass/settings.h:35](/Users/gabeochoa/p/pharmasea/src/dataclass/settings.h:35) (`ConfigKey`), [project_context.md](/Users/gabeochoa/p/pharmasea/project_context.md) (tag-over-bool convention).

## 6. Open questions (need owner decision — do not guess-fix during FSM passes)

1. **M20 serialization:** serialize `IsPnumaticPipe::recieving/item_id` (save bump) or reconstruct on load? Affects whether in-flight pipe items survive save/host-sync.
2. **M05 counters:** is resetting `num_orders_rem/tab/tip` on load (only `current_order/order_state` serialized) intentional? If not, that is a save fix separate from FSM explicitness.
3. **M06 orphan occupant:** loaded `InUse` toilet has `user=-1` (field unserialized). Clamp to `Available` on load, or serialize `user`?
4. **M16 `working`:** wire it (set on `update()`), or delete the field (save bump)? Current code suggests an unfinished feature, not a bug with known intent.
5. **M02 step 2:** split `menu::State::UI=99` into a separate `InputLayer` enum now, or only if a second input-layer consumer appears? (Recommendation: defer; step-1 table first.)

## 7. Phased rollout (behaviour-preserving; build must stay green)

Verification legend: **B** = `make` (repo root) succeeds · **T** = `./pharmasea.exe --tests-only` passes ([src/tests/all_tests.h](/Users/gabeochoa/p/pharmasea/src/tests/all_tests.h); tests use `app.expect_*`/`wait_for_*`, no branching) · plus the per-phase soak. Do not combine phases for the same machine.

### Phase 0 — Guards without behaviour change (P0 safety, all S)

- [x] M20: add derived `PairingState` accessor + log on transfer-state load (no serialize change yet; §6.1 decision requested)
- [x] M09: add derived `session_state()` + `log_warn` when `desired_role` holds both Host and Client bits
- [x] M11: add `SlotState`/`LoadResult` enums alongside existing bools; route trigger/CLI callers to log the specific failure
- [x] M10: add `to_connection_state()` mapper with exhaustive switch; call it (log-only) from both Steam callbacks
- [x] M03: add derived `HasDayNightTimer::phase()`; point `system_utils::should_run_*` at it (single site, no stored state)
- Verify: B + T + host/join/kill-host soak (GNS and LOCAL_ONLY) + save→load round-trip

### Phase 1 — Core flow tables (P1)

- [ ] M01: `GameState::transition_to()` edge table (log-only) → route trigger/layer/network writers → deprecate raw `set()`
- [ ] M02: `MenuState` transition table + host-sync allowlist `{Network→Game}` (fixes the settings-override class, not just the one edge)
- [ ] M13: assert single `set_active_layer` call site (rides M02; no separate change)
- [ ] M07: derived trigger `Phase` accessor for render/debug; keep floats/tags as truth
- [ ] M04: private `state` + accessor; `is_legal(from,to)` log-only check in commit; route `set_state_immediately` bypasses (spawn-init exempt)
- Verify: B + T + pause/resume + two-client settings soak + full lobby→game→progression→store loop

### Phase 2 — Local machines (P2, each independent)

- [x] M05: `enum class` + `transition_to()` + M04↔M05 coupling check (debug assert)
- [ ] M06: `enum class` (same values) + `end_use` guard; resolve §6.3
- [ ] M08: derived `ProgressionPhase` + `advance_cycle()` sole index writer
- [ ] M12: `CaptureState` replaces popup `show` bool
- [ ] M14: derived fishing `Phase`; bar/feedback readers use it
- [x] M15: derived `WorkPhase`; progress-bar readers use it
- [ ] M16: derived squirter `Phase`; resolve §6.4 separately
- [ ] M18: sentinel named constants + derived `ContainerState` (debug/render only)
- [ ] M19: debug invariant holder↔item back-pointer check in `CanHoldItem::update`
- [ ] M21: `NuxPhase` derived + driver switch (step table only if tutorial work is already scheduled)
- Verify per item: B + T + the machine’s soak listed in its record

### Phase 3 — Stored enums + save bumps (only with `save_version` bump, one machine per change)

- [ ] M03: store `Phase`, append last in `serialize()`, bools become derived shims → delete shims next release
- [ ] M07: store trigger `Phase`; move delete-mode to room entity (`LoadSaveMode`)
- [ ] M09: store `SessionState`; `desired_role` becomes boundary shim (optional M10-driven `ConnectionLost` UX follows, flagged behaviour change)
- [ ] M14/M21: store phases if Phases 0–2 prove the derived forms insufficient (default: skip — derived is enough)
- Verify per item: B + T + old-save load probe (previous `save_version`) + host/client sync soak

### Phase 4 — Trivial / dev-only (P3, anytime, no soak)

- [x] M17: `IsSpawner::state()` accessor
- [ ] M22: `BypassPhase` + `advance()` (dev harness)
- [ ] M23: optional `IntroPhase` log names only
- [ ] M24: `ReplayPhase` log names only (file format untouched)
- Verify: B + T

Explicitly **not** in any phase: renumbering any enum in §4–§5; touching `vendor/afterhours`; converting §5 keep-bool rows; behaviour changes listed as “optional/flagged” (pause-as-data in M01, Settings yank fix beyond allowlist in M02, ConnectionLost UI in M09, slot labels in M11) — each needs its own approved change after the preserving step lands.
