---
kanban-plugin: basic
---

# Pub Panic! Task Tracker

**Quick Stats:**
- Priority Fixes: 7 bugs + 4 UX issues
- Core Features: 6 items
- Save/Load: 6 items
- ECS Refactor: 4 items
- Afterhours Migration: 10 items (3 done)
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

**Phase 1 - Handle system in afterhours:**
- [ ] Implement `EntityHandle` + slots/free-list/dense-index in `vendor/afterhours`
- [ ] Add `id_to_slot` and make `getEntityForID` O(1)
- [ ] Rewrite cleanup/delete paths to keep slots/dense indices correct and bump generation
- [ ] Add `EntityQuery::gen_handles()` and `gen_first_handle()`

**Phase 2 - Migrate pharmasea:**
- [ ] Audit and migrate remaining serialized entity relationships to handles

**Phase 3 - Snapshot DTOs:** (mostly done)
- [x] Add snapshot DTO layer (`world_snapshot_blob`)
- [x] Switch save-game to snapshot payloads
- [x] Remove `PointerLinkingContext` from network and save contexts
- [ ] Add save versioning + migration hooks

**Decisions to lock down:**
- [ ] Handle layout: prefer `uint32_t slot/gen` for stable wire format
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
