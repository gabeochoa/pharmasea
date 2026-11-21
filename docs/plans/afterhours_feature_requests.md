# Afterhours feature requests: what the engine should absorb next

> Research report (read-only except this file). Goal: name **new** engine
> features that let pharmasea — and other games like it — delete large
> hand-rolled subsystems while gaining capability. Written from the
> engine designer's seat: every ask is stated in its generic shape first,
> with pharmasea as the worked example.
>
> Method: read `afterhours_gaps.md` in full, `docs/active/todo.md`
> (including "Skip / not moving" and the vendor move-over sections), the
> vendor plugin inventory under `vendor/afterhours/src/`, and the largest
> local subsystems under `src/`. Line counts below are `wc -l` measured
> in this session unless marked *(est.)*; deletion figures are estimates
> and are marked as such. Estimates overlap where noted — do not sum the
> table naively.

## What this report deliberately does not re-ask

Already tracked in [afterhours_gaps.md](/Users/gabeochoa/p/pharmasea/afterhours_gaps.md) and excluded here:

| Existing ID | Topic (excluded) |
| --- | --- |
| G-P1–G-P6 | Online session adapter, snapshot sampler, DNS, admission hook, GNS distribution, Steam split |
| G-P7 | Intro cards / loading screens |
| G-P8 | Instancing tint / transform / shader-ownership helpers |
| G-P9 | E2E time-scale reference wiring |
| G-P10 | Headless default font |
| G-P11 | Countdown form of `timer::HasTimer` with a serialize hook (narrow timer ask only — see §6 for the distinct, larger trigger/progress ask) |
| G-P12 | Portable seeded RNG (explicitly skipped per brief) |
| G-P13 | Pathfinding serialize hook + grid neighbor provider |
| G-P14 | Arena/allocator-backed query results |
| G-E*, G-M* | E2E, multiplayer, save-on-new-transport adapter work (game-side by design) |

Also excluded because [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) records them as done, evaluated, or deliberately kept local: frustum, LOD, instancing pilot, `FrameRateStats` e2e readout, `files::TestRoots`, E2E time-scale substepping, headless NONE backend, the local toast manager staying local (`Skip (recorded in report): … toast plugin (local toastmanager.h stays)`), and the M01–M24 explicit-FSM conversions staying in pharmasea (`Keep local (FSM, per fsm_todo.md …): vendor afterhours stays untouched`). §7 and the "deliberately not engine" list respect those calls — they ask for narrow shared utilities *below* the FSM, not an FSM framework.

Two vendor facts shape several asks below and are worth stating once:

- The vendor UI is already large and capable — `vendor/afterhours/src/plugins/ui/` totals ~37k lines across headers measured in this session, including [imm_components.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_components.h), [setting_row.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/setting_row.h) (495 lines), [menu.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/menu.h) (468 lines), [imm_virtual_list.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_virtual_list.h) (421 lines), plus [modal.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/modal.h) (1,128 lines) and [toast.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/toast.h) (361 lines). Pharmasea calls **none** of the `imm::` API: a search for `imm::` / `ui::setup` / `ComponentConfig` / `mk(` across `src/layers`, `src/system/ui`, and `src/engine/ui` returned no hits. It instead maintains a parallel UI stack in `src/engine/ui/` (3,912 lines measured: [color.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/color.h) 979, [ui.cpp](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui.cpp) 618, [autolayout.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/autolayout.h) 515, [svg.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/svg.h) 294, [ui_input_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_input_manager.h) 224, [ui_internal.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_internal.h) 220, [focus.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/focus.h) 156, [rect.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/rect.h) 152, and smaller pieces). The asks in §1–§2 are therefore **not** "build widgets" — the widgets exist. They ask for the missing binding/scaffolding layer above them, plus a supported migration bridge so a game with an existing rect-based UI can move screen-by-screen instead of in one rewrite. That is also why the [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) "imm UI suite" skip does not settle §1: the skip declined adopting the suite wholesale; it did not evaluate a data-bound form/list layer, which does not exist upstream today.

- Pharmasea's local code totals ~65k lines under `src/` (64,967 measured, of which 12,415 are the generated `src/resources/fonts/Karmina_Regular_256.h` header — so ~52.5k hand-written lines, *(est.)* after that single exclusion). The subsystems ranked below are the largest hand-written clusters in that total.

---

## (a) Ranked top 10

| # | Feature (generic shape) | Local code it deletes (approx) | Features it unlocks | Other-games appeal (1–3) |
| --- | --- | --- | --- | --- |
| 1 | Data-bound UI scaffolding on the imm suite: form schema → rows, data-bound lists, menu/settings screen shells | ~1,100 lines *(est., net; overlaps #2 at the settings screen — counted once in §2)* | New screens in tens of lines; gamepad focus, themes, and localization handled once; HUD lists (lobby, recipes, orders) become bindings, not draw loops | 3 |
| 2 | Settings + key-remapping service: version-tolerant settings store and a press-any-key remap widget with conflict UI | ~800 lines *(est., net; shares `settingslayer.cpp` with #1)* | Every game gets a working settings + controls screen; remap popup (currently disabled) returns; settings survive field additions | 3 |
| 3 | Prefab / component-bundle authoring: named bundles + spawn-with-overrides, usable from code or data | ~700 lines *(est., of the 1,719-line maker file)* | Data-driven content, editor/test spawning by name, mods; new furniture/items stop being new C++ functions | 3 |
| 4 | Save-game versioning & migration framework for snapshot saves | ~350 lines *(est., net)* plus every future fixer's worth of ad-hoc code | Old saves load across releases instead of `VersionMismatch` rejection; component additions stop being save-breaking events | 3 |
| 5 | Attachment system: one parent→child hold/attach relation with sockets, filters, and held-pose solving | ~450 lines *(est., spread across components, input, and held-pose systems)* | Hand-trucks, ropes, carried items, turret/passenger mounts all become one mechanism; pickup/drop stops being per-pair code | 3 |
| 6 | Trigger-volume + progress family: entrant counting, fill/cooldown phases, validation, and world/screen progress rendering as one plugin | ~600 lines *(est.)* | "Stand here together to start / buy / save" and timed work stations become declarations; progress bars stop being re-drawn per game | 3 |
| 7 | NPC queue-interaction helper: join / advance / interact-at-front / leave as a reusable behavior utility | ~300 lines *(est.; the game's own plan estimates ~270)* | Any line/queue behavior (register, ticket, bus stop) in one config; queue bugs fixed once | 2 |
| 8 | World-space labels: a `WorldLabel` component (text/icon, billboarding, distance fade) rendered by the engine | ~300 lines *(est., including debug-label duplication)* | Speech bubbles, nameplates, patience/order icons, and debug labels share one billboard-correct path (fixes a filed priority bug) | 3 |
| 9 | Tile-grid procgen toolkit: seeded grid, WFC solver, and required-placement / playability validation | ~700 lines *(est., of ~1,900 lines of map generation)* | Roguelikes, builders, and room-games get generation + "is this map playable?" checks without re-deriving WFC | 2 |
| 10 | Live debug inspector: entity picker, component view/edit, system timings in one overlay | ~300 lines *(est., across debug layers and floating-text debug draws)* | Every ECS game gets the "click the thing, see its components" loop; fewer one-off debug layers per game | 3 |

---

## (b) One section per item

### 1. Data-bound UI scaffolding on the imm suite

**Today.** UI is hand-laid-out immediate mode over manual rectangle carving. [rect.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/rect.h) (152 lines) provides only `lpad/rpad/tpad/bpad`, `vsplit`/`hsplit` in 2- and N-way forms — every screen then composes dozens of splits by hand. [settingslayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.cpp) (551 lines) is the clearest specimen: `draw_base_screen` splits one window into 12 rows, then repeats the same label/control block twelve times — `vsplit(row, 30)`, `text(Widget{label}, TranslatableString(...))`, then `slider`/`dropdown`/`checkbox(Widget{control}, ...)` wired field-by-field into `Settings::get()`. The widget implementations themselves live in [ui.cpp](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui.cpp) (618 lines: `button`, `checkbox`, `slider`, `dropdown`, `textfield`, focus/tab handling), backed by [autolayout.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/autolayout.h) (515), [focus.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/focus.h) (156), [ui_input_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_input_manager.h) (224), and [theme.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/theme.h) (91). Screens repeat the pattern: [networklayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/networklayer.cpp) (406 lines) hand-draws a lobby player list by advancing `player_name.y += height_per` in a loop; [ui_rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ui/ui_rendering_system.cpp) (515 lines) hand-draws HUD order cards, round timers, rent bars, and bank balances with raw `raylib::DrawRectangleRounded` / `DrawCircle` calls; [menulayer.h](/Users/gabeochoa/p/pharmasea/src/layers/menulayer.h) drives the main menu through an SVG-positioned [svg.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/svg.h) helper (294 lines). Across `src/layers` alone there are 182 `rect::` / `button(` / `slider(` / `dropdown(` / `checkbox(` call sites measured in this session. Localization rides along manually: every label is a `TranslatableString(strings::i18n::...)` from [strings.h](/Users/gabeochoa/p/pharmasea/src/strings.h) (522 lines), while the vendor already ships [translation.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/translation.h) (372 lines) with a compatible `TranslatableString` template that pharmasea does not use.

**What's missing upstream.** The vendor has the leaf widgets — including exactly the right leaf for settings, [setting_row.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/setting_row.h), which renders `[icon] label … [toggle|stepper|slider|dropdown]` from a `SettingRowConfig` + value pointer — and list/menu/modal primitives. What it does not have is the layer that turns a *table of bindings* into a screen: no form descriptor, no "render this vector as rows" binding beyond the [imm_virtual_list.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_virtual_list.h) scrolling primitive, no settings-screen shell (title, scrollable body, footer with save/back/reset), and no bridge that lets an existing `Widget{Rectangle}`-style game adopt imm one screen at a time.

**Generic API sketch.**

```cpp
Form form{ctx, parent};
form.slider("Master Volume", data.master_volume);          // label from i18n key
form.dropdown("Language", lang_options, data.lang_index);  // options + index binding
form.toggle("Fullscreen", data.isFullscreen, [&]{ apply(); });
form.list(players, [](auto& row, Player& p){ row.text(p.name); row.icon(p.mug); });
```

**Deletes / unlocks.** Net *(est.)* ~1,100 lines: most of `src/engine/ui/` widget/layout/focus code becomes vendor code over time, and each screen sheds its split-and-wire repetition (the 12 near-identical settings blocks collapse to 12 one-liners). Unlocks: screens become cheap enough to add (a real pause menu, a recipe book, a save-slot picker are currently each a new hand-laid-out layer), focus/gamepad navigation and theme changes apply uniformly, and text measurement for localization stops being per-widget. **Other games:** every game with menus and settings re-derives this exact layer; appeal 3.

---

### 2. Settings + key-remapping service

**Today.** Persistence and remapping are three separate hand-rolled stacks. (i) Settings data: [settings.h](/Users/gabeochoa/p/pharmasea/src/engine/settings.h) (186 lines) defines `settings::Data` with a positional `zpp::bits` archive and carries the telling comment `TODO How do we support multiple versions / we dont want to add a new field and break / all past save games`; [settings.cpp](/Users/gabeochoa/p/pharmasea/src/engine/settings.cpp) (343 lines) implements load/save and a dozen `update_*_from_index` / `get_*_options` pairs. (ii) Key bindings: [keymap.h](/Users/gabeochoa/p/pharmasea/src/engine/keymap.h) (181 lines) defines `InputName` (~30 actions), `AnyInput = variant<int, GamepadAxisWithDir, GamepadButton>`, and `FullMap = map<menu::State, LayerMapping>` — bindings are per-menu-state layers; [keymap.cpp](/Users/gabeochoa/p/pharmasea/src/engine/keymap.cpp) (346 lines) plus [input_mapping_persistence.h](/Users/gabeochoa/p/pharmasea/src/input_mapping_persistence.h) (115) and [input_mapping_setup.h](/Users/gabeochoa/p/pharmasea/src/input_mapping_setup.h) (93) handle JSON round-tripping. (iii) The remap UI: the second half of [settingslayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.cpp) rebuilds label/icon lookups per input (`_get_label_for_key`, `_get_icon_for_gamepad`, …) and the actual capture popup is commented out — [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) records `the remap popup itself is still disabled in settingslayer.cpp`, with only the conflict check (`KeyMap::conflicts_in` → vendor `input::actions_using`) landed. The vendor already provides the two halves separately — [settings.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/settings.h) (468 lines: `ProvidesSettings<SettingsData>` with JSON/bitsery/raw load/save and `on_change` callbacks) and [input_system.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/input_system.h) (1,546 lines: polling, `name_for_input`/`icon_for_input`, gamepad mappings) — but nothing joins them into "a settings file that tolerates new fields" + "click Remap, press a key, see conflicts" that a game can drop in.

**Generic API sketch.**

```cpp
settings::store<MyData>("settings.json").version(3)
  .migrate(1, [](auto& d){ d.vsync_enabled = true; }).load();
auto r = remap_capture(ctx, parent, actions, Action::Jump); // press-any-key + conflicts
if (r) bindings.set(ctx_state, Action::Jump, *r);
```

**Deletes / unlocks.** Net *(est.)* ~800 lines across the three stacks (sharing `settingslayer.cpp` with §1 — the settings-screen rows are counted there, the store/remap machinery here). Unlocks: adding a setting stops being a save-breaking event, the disabled remap popup returns with conflict refusal built in, and keyboard/gamepad icon+label lookup (currently re-derived per screen) comes from the input plugin. **Other games:** settings + remapping is universal pre-game work; appeal 3.

---

### 3. Prefab / component-bundle authoring

**Today.** [entity_makers.cpp](/Users/gabeochoa/p/pharmasea/src/entities/entity_makers.cpp) is 1,719 lines — the largest hand-written file in `src/` — of `make_*` functions that repeat the same choreography: `make_entity` → `addComponent<Transform>` → renderer choice (`ModelRenderer` if `ENABLE_MODELS` and the model exists, else `SimpleColoredBoxRenderer`) → capability components (`CanHoldItem`, `CanBeHeld`, `IsSolid`, …) → a `HasWork().init(lambda)` for work stations. The game's own [data-driven-entity-makers.md](/Users/gabeochoa/p/pharmasea/docs/plans/data-driven-entity-makers.md) plan inventories the repetition (8 item-container functions ~120 lines, 3 ingredient-adder functions ~80, 8+ identical `HasWork` wirings) and estimates 400–500 lines saved by game-side specs alone; [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) carries the infra item `PrefabLibrary (data-driven entity builders): move repetitive makers to JSON prefabs`. A search of `vendor/afterhours/src/plugins/` for prefab/bundle facilities returned nothing — the ECS has components and queries but no bundle, archetype, or named-spawn concept.

**Generic API sketch.**

```cpp
prefabs::define("ice_machine", furniture_base, item_container{IceBag, 4},
                ingredient_adder{Ingredient::Ice}, work_station{1.5f});
Entity& e = prefabs::spawn("ice_machine", vec2{3, 5});
e.get<HasWork>().reset_speed(2.f);  // ordinary override after spawn
```

**Deletes / unlocks.** *(Est.)* ~700 lines of makers collapse into bundle definitions + a registry; the renderer-fallback and capability boilerplate is written once per bundle, not once per entity type. Unlocks: content defined in data (JSON or constexpr tables) that designers/tests/e2e can spawn by name, and a natural home for the "which components does an X have" question that save/network code also asks. The plan file's open questions (compile-time vs runtime, serialization impact) are exactly engine-level decisions — component bundles must compose with the existing `serialize` field-order lists, not bypass them. **Other games:** every ECS game writes `make_goblin()`; appeal 3.

---

### 4. Save-game versioning & migration framework

**Today.** Saves are a versioned envelope with no migration path. [save_game.h](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h) (143 lines): `SaveGameHeader{ save_version = 1, hashed_build_version, … magic = "PHARMSAVE" }`, and `SlotInfo::state()` returns `VersionMismatch` whenever `save_version` or the build hash differs — old saves are rejected, not migrated. [save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp) (264 lines) adds atomic write-via-temp+rename, header-only reads, and slot enumeration, guarded by `kExpectedComponentTypesChecksum` with a `static_assert` forcing a human to acknowledge any `ComponentTypes` change. The world payload is [world_snapshot_blob.cpp](/Users/gabeochoa/p/pharmasea/src/serialization/world_snapshot_blob.cpp) (295 lines): per-entity version word (`kEntitySnapshotVersion = 4`), a component-presence bitset serialized in stable `ComponentTypes` tuple order, then positional `zpp::bits` payloads — adding a field mid-tuple silently shifts every later field. Repairs live in [post_deserialize_fixups.cpp](/Users/gabeochoa/p/pharmasea/src/post_deserialize_fixups.cpp) (88) and [save_file_load_fixers.cpp](/Users/gabeochoa/p/pharmasea/src/save_file_load_fixers.cpp) (112). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) leaves `Add save versioning + migration hooks` unchecked under Phase 3, and [afterhours_gaps.md](/Users/gabeochoa/p/pharmasea/afterhours_gaps.md) G-M12 records that save, network sync, and the component checksum share one blob — so the format question cannot be solved game-locally without re-solving it for sync too.

**Generic API sketch.**

```cpp
save::envelope<MyHeader>(/*current=*/7)
  .migrate(5, [](auto& a){ a.read_old<Pos>(); a.write_new<Transform>(); })
  .migrate(6, [](auto& s){ s.rename_component("speed", "base_speed"); });
auto loaded = save::load_slot(3);  // Ok | Migrated{from=5} | Corrupt | TooNew
```

**Deletes / unlocks.** *(Est.)* ~350 lines net (the manager's classify/load plumbing plus both fixer files become registered migrations), and — more importantly — every *future* fixer stops being a new file. Unlocks: shipping content updates without wiping players' saves, and a single place where "what changed in the format" is answered for saves and network snapshots alike. The engine is the right owner because the mechanism (envelope version, ordered migration chain, too-new/corrupt discrimination, header-only preview reads) is format-agnostic; only the per-version lambdas are game code. **Other games:** any game with saves and evolving components; appeal 3.

---

### 5. Attachment system (held items, furniture, hand-trucks, ropes)

**Today.** "X holds Y" is re-implemented per pair. Components: [can_hold_item.h](/Users/gabeochoa/p/pharmasea/src/components/can_hold_item.h) (133 lines, holder↔item dual bookkeeping with `held_by`/`held_by_id` sentinels), [can_hold_furniture.h](/Users/gabeochoa/p/pharmasea/src/components/can_hold_furniture.h) (45), a `CanHoldHandTruck` component, [has_rope_to_item.h](/Users/gabeochoa/p/pharmasea/src/components/has_rope_to_item.h) (52, a `vector<EntityRef>` chain), `ConveysHeldItem`, and [custom_item_position.h](/Users/gabeochoa/p/pharmasea/src/components/custom_item_position.h) (43) for per-type held poses. Logic: grab/drop/rotate/place-validity lives in [input_process_manager.cpp](/Users/gabeochoa/p/pharmasea/src/system/input/input_process_manager.cpp) (969 lines — `handle_grab_or_drop`, `drop_held_furniture`, `work_furniture`, each re-deriving reach, walkability, and store-boundary rules, with `TODO :PICKUP: i dont like that these are spread everywhere` at the sound call sites), and held-pose solving in `src/system/sixtyfps/` (`afterhours_held_position_helpers.h` 96 + `held_position_traits.h` 53, inside a 1,258-line directory total). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) already names the consolidation: `AttachmentSystem: unify held item/furniture/handtruck/rope via generic parent→child attachment`. No attachment/parenting facility exists in the vendor plugin inventory.

**Generic API sketch.**

```cpp
attach::to(child, parent, {.socket = "hand", .filter = {Item, Furniture}});
attach::detach(child);                       // pose + velocity inherited once
if (auto held = attach::held_by<Drink>(player)) use(*held);
```

**Deletes / unlocks.** *(Est.)* ~450 lines: one relation replaces four component shapes, the per-pair grab/drop code paths, and the duplicated held-pose traits; invariants (a child has one parent; cleanup detaches) are checked once. Unlocks: new carryables (a mop buddy is already special-cased as a droppable "special boy" in `handle_drop`) stop needing new code paths, and rope/chain following falls out of nesting attachments. **Other games:** carrying, mounting, parenting, and socket attachment are near-universal; appeal 3.

---

### 6. Trigger-volume + progress family

*Distinct from G-P11:* that ask is only "give the vendor countdown timer a serialize hook." This section asks for the whole interaction pattern built on top of timers — entrant accounting, activation phases, validation, and rendering — which today is a game subsystem, not a timer.

**Today.** [is_trigger_area.h](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h) (264 lines) encodes a 13-type `Type` enum (lobby start, store reroll, load/save slots, planning save…), an `EntrantsRequired::{All, AllInBuilding, One}` policy, a derived `Phase::{Idle, Filling, Full, Cooldown}` from two float pairs, a `ValidationFn` callback whose result must be cached in a `mutable` field because `std::function` cannot serialize, and title/subtitle plumbing. Around it, `src/system/trigger/` totals 981 lines: [trigger_cb_on_full_progress_system.h](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_cb_on_full_progress_system.h) (361), [trigger_area_systems.cpp](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_area_systems.cpp) (267), [update_dynamic_trigger_area_settings_system.h](/Users/gabeochoa/p/pharmasea/src/system/trigger/update_dynamic_trigger_area_settings_system.h) (188), plus entrant counters. The work/progress side is [has_work.h](/Users/gabeochoa/p/pharmasea/src/components/has_work.h) (106: `pct_work_complete` + `WorkFn` callback + progress-bar visibility flags) and [cooldown_info.h](/Users/gabeochoa/p/pharmasea/src/components/cooldown_info.h) (57), and progress *rendering* is re-derived in [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (`ProgressBarConfig`, `DrawProgressBar`, `DrawFishingGame`, ~150 lines of cube-based bars). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) lists both infra items explicitly: `Progress/Cooldown components + ProgressRender: centralize timers/progress bars` and `TriggerAreaSystem: own entrants counting, validation, progress/cooldown and activation`.

**Generic API sketch.**

```cpp
e.addComponent<TriggerVolume>({.radius = 1.5f, .need = Entrants::AllPlayers,
                               .fill_seconds = 2.f, .validate = &can_start});
on_trigger_full<StartGame>([](Entity& zone){ begin_round(); });
progress::bar3d(entity, work.pct());  // world bar; progress::bar2d for HUD
```

**Deletes / unlocks.** *(Est.)* ~600 lines: entrant counting, phase derivation, and bar rendering move into the plugin; the game keeps its `Type` enum, validation lambdas, and on-full callbacks. Unlocks: "stand in the circle together" — the lobby/store/save interaction this game uses everywhere — becomes a declaration, and multiplayer entrant semantics (who counts, client vs server validation) are solved once. **Other games:** capture points, elevators, crafting stations, and co-op start pads are the same shape; appeal 3.

---

### 7. NPC queue-interaction helper

*Scope guard:* [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) and `fsm_todo.md` keep the M01–M24 FSM conversions game-side, and this report does not propose an engine FSM framework (see "deliberately not engine"). What is shared, and small enough to be a utility rather than a framework, is the queue behavior repeated across AI states.

**Today.** Customer AI is a 9-state machine in [is_ai_controlled.h](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h) (168 lines: legal-transition table, staged `next_state`, `resume_state`, ability flags) with per-state data components (`HasAIQueueState`, `HasAIDrinkState`, `HasAIBathroomState`, …) and per-state systems under `src/system/ai/` (1,683 lines total measured for the directory's headers/cpp in the earlier pass; individual systems: bathroom 144, jukebox 126, at-register 113, clean-vomit 111, drinking 102, pay 96, queue-for-register 78, wander 66). Queue manipulation itself is centralized only halfway: [ai_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ai/ai_system.cpp) (297 lines) holds `line_add_to_queue` / `line_try_to_move_closer` / `line_leave` helpers that the register, pay, bathroom, and jukebox systems each drive with their own find-target/interact/leave choreography. The game's own [generic-ai-queue-system.md](/Users/gabeochoa/p/pharmasea/docs/plans/generic-ai-queue-system.md) plan puts four systems at ~427 lines with an identical find → join → advance → interact → leave → transition skeleton and estimates ~270 lines saved by extracting the shared utilities (its recommended Approach A).

**Generic API sketch.**

```cpp
queue::Behavior{.queue = register_queue, .at_front = &serve_customer,
                .done_state = State::Drinking};
queue::tick(npc, queue, dt);  // join, shuffle forward, fire at_front once
```

**Deletes / unlocks.** *(Est.)* ~300 lines (the plan's ~270 plus per-system target/leave glue). Unlocks: new queued behaviors (a second register, a ticket window, a bus stop) become a config + one callback, and line bugs (the day-3 "people got stuck in line" class in [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md)'s completed list) get one fix site. The state enum, transition legality, and per-state decisions stay game-side, per the FSM decision. **Other games:** any NPC that lines up; appeal 2 (common, but not universal).

---

### 8. World-space labels (billboard text/icons)

**Today.** Floating text is drawn imperatively wherever it is needed. [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (1,596 lines) alone contains `DrawFloatingText` call sites for entity IDs, debug subtypes, drink contents, uses-remaining, and AI state names (`render_debug_*` family), plus a hand-rolled `draw_billboard_from_texture_or_atlas` that probes four atlases then falls back to the texture library. Gameplay labels ride other components ad hoc: `HasSpeechBubble` gates order icons in [ui_rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ui/ui_rendering_system.cpp), `HasPatience::pct()` drives card colors, and [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) files both the bug (`Text doesnt rotate based on the camera (should billboard properly)`) and the desired shape (`BillboardText/WorldLabel component + system`). The vendor has text pieces ([curved_text.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/curved_text.h), text units/stroke under `plugins/ui/`) but no world-anchored label component with billboarding, occlusion/distance policy, and icon+text composition.

**Generic API sketch.**

```cpp
e.addComponent<WorldLabel>({.text = "2x Mojito", .icon = "mojito",
                            .billboard = true, .fade_beyond = 12.f});
labels::set_text(e, fmt::format("{}s", patience_left));  // engine draws it
```

**Deletes / unlocks.** *(Est.)* ~300 lines: the debug-label family and per-feature floating-text draws collapse into data on components, and billboarding is fixed once, in the engine, for every label. Unlocks: nameplates, speech/order bubbles, damage numbers, and tutorial prompts become component edits instead of render-system surgery. **Other games:** every 3D game floats text over things; appeal 3.

---

### 9. Tile-grid procgen toolkit

**Today.** Map generation is ~1,900 lines of game code with a generic core buried in it: [map_generation.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation.cpp) (740), [simple.h](/Users/gabeochoa/p/pharmasea/src/map_generation/simple.h) (790, under `src/map_generation/`), [wave_collapse.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/wave_collapse.cpp) (536) + header (159), [ascii_grid.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/ascii_grid.cpp) (243), [pipeline.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/pipeline.cpp) (121), [day1_required_placement.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/day1_required_placement.cpp) (145), [day1_validation.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/day1_validation.cpp) (58), and [playability_spec.h](/Users/gabeochoa/p/pharmasea/src/map_generation/playability_spec.h) (198). The generic parts — a seeded tile grid, a Wave Function Collapse solver over adjacency rules, ASCII layout stamping, and "required items placed + reachable" validation — are interleaved with bar/store building shapes. The vendor inventory has no grid/WFC/procgen plugin (pathfinding exists, G-P13 aside; it consumes walkability, it does not generate it). Seeding stays as-is per the brief (G-P12 excluded); the toolkit would accept a caller-supplied RNG stream.

**Generic API sketch.**

```cpp
auto grid = procgen::wfc(tileset, {12, 12}, rng_stream("layout"));
procgen::stamp(grid, ascii_layout, legend);
auto report = procgen::validate(grid, required({Register, 1}, {Door, 2}), reachable_from(entrance));
```

**Deletes / unlocks.** *(Est.)* ~700 lines: the WFC solver, grid, stamping, and reachability/required-placement validators become engine code with tests; pharmasea keeps its tileset, building definitions, and day-1 content rules as data. Unlocks: deterministic generation checks (the `run_determinism.sh` property — same seed, byte-identical map) become an engine-tested guarantee rather than per-game discipline. **Other games:** roguelikes, builders, and room-based games share the core, but plenty of games never generate maps; appeal 2.

---

### 10. Live debug inspector

**Today.** Debugging is a pile of one-off layers and in-world text. [gamedebuglayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/gamedebuglayer.cpp) (143) + header, [debug_settings.h](/Users/gabeochoa/p/pharmasea/src/layers/debug_settings.h) (194), [uitestlayer.h](/Users/gabeochoa/p/pharmasea/src/layers/uitestlayer.h) (83), [fpslayer.h](/Users/gabeochoa/p/pharmasea/src/layers/fpslayer.h), and the `render_debug_*` floating-text family in [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (entity IDs on bounding boxes, AI state names, drink contents) each answer one question with bespoke draw code. The vendor has the ingredients — [layout_inspector.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/layout_inspector.h) (289) for UI, [debug_overlay.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/debug_overlay.h), [profiling.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/profiling.h) (298), `system_timing.h` — but no ECS inspector: no "click an entity in the world, see/edit its components, see which systems touched it" loop.

**Generic API sketch.**

```cpp
inspector::enable(systems);            // F-key overlay, world picking built in
inspector::watch<HasPatience>("pct");  // pin a field to the HUD watch list
// click entity -> component tree -> edit field -> change applies next tick
```

**Deletes / unlocks.** *(Est.)* ~300 lines of debug layers and debug draws (replaced by registrations), and whole categories of "add a debug print, rebuild, remove it" cycles. Unlocks: designers and testers can answer "why is this customer stuck?" by clicking the customer — the question the AI debug text exists for — without a build. **Other games:** every ECS game rebuilds this; appeal 3.

---

## (c) Deliberately not engine

Things that look generic from a distance but should stay in the game, with the reason in one line:

| Candidate | Why it stays game-side |
| --- | --- |
| Drink recipes, ingredient algebra, order validation (`validate_drink_order`, mocktail/drunk-tolerance upgrades in [ai_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ai/ai_system.cpp)) | It *is* the game's rules; no second game shares Mojito xor-counting. |
| Customer decision policy (bladder → bathroom, patience, tipping, when to leave) | Tuning and identity of this game; the engine contribution is §7's queue utility, not the decisions. |
| The explicit FSM conversions M01–M24 (game flow, day/night phase, toilet, fishing, NUX…) | Already decided in `fsm_todo.md`: transition tables are small, serialized-value-sensitive, and game-owned; only the staged-commit *pattern* (§7) is shared. |
| Bar/store economy: progression options, store stock, rent, pricing (`IsProgressionManager`, `IsBank`, `progression.h`) | Balance/content, not mechanism; an engine "shop system" would encode this game's economy. |
| Map content: building shapes, day-1 required placements, tileset art rules | Content data for §9's toolkit; the solver is generic, the bar is not. |
| Network packets, authority rules, and who-may-save/seed checks | [afterhours_gaps.md](/Users/gabeochoa/p/pharmasea/afterhours_gaps.md) already assigns schemas/authority to the game (G-M1–G-M3, G-M7); the transport stays dumb on purpose. |
| Tutorial/NUX step content and bypass automation | Ordered, hand-tuned onboarding scripts; an engine sequencer would be a worse authoring tool than the current step list. |
| Fishing minigame scoring, squirter/drink-work callbacks in `HasWork().init(...)` lambdas | Minigame rules and per-station behavior; §6 provides the progress shell, the callbacks remain the game's. |
| Intro art, logo cards, and theme assets | Already covered as an engine ask by G-P7 for the *runner*; the artwork and brand timing are the game's. |
| Sound/music selection (`SoundId`, when PICKUP vs PLACE plays) | Content and game feel; the vendor sound system already owns playback. |

## (d) The 3 to build first

1. **§1 Data-bound UI scaffolding** — it is the largest delete (~1,100 lines *(est.)*), it unblocks §2's settings screen and every future menu/HUD, and the leaf widgets already exist upstream, so the work is the binding layer and the migration bridge, not a new widget library. Every subsequent UI the game adds gets cheaper the day this lands.
2. **§3 Prefab / component-bundle authoring** — content velocity is the game's other tax: 1,719 lines of makers for what is mostly data. Bundles also force the component-composition questions that §4 (save migrations) and §5 (attachments) need answered, so building this second de-risks both.
3. **§4 Save-game versioning & migration framework** — it is the safety gate for everything else: prefab and attachment work will add/reorder components, and today that invalidates saves (`VersionMismatch`) and trips the checksum `static_assert`. Landing migrations early means the later refactors ship without wiping players, and the same envelope serves the network snapshot that shares the blob.

§2 (settings/remap service) is the natural fourth: it rides on §1's form scaffolding, and its store half can proceed in parallel since it has no UI dependency beyond the remap widget.

---

*Evidence note: every path and line count above was opened or measured (`wc -l`) in this session; figures marked *(est.)* are deletion estimates derived from those counts and from the game's own plans ([data-driven-entity-makers.md](/Users/gabeochoa/p/pharmasea/docs/plans/data-driven-entity-makers.md), [generic-ai-queue-system.md](/Users/gabeochoa/p/pharmasea/docs/plans/generic-ai-queue-system.md)), not measurements of a finished migration. No file other than this one was modified.*
