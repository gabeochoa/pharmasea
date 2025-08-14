---

kanban-plugin: basic

---

## backlog

- [ ] WARN: need a way to warn that UI elements are offscreen "purpling"
- [ ] Add "powered by raylib" intro card (and other intro cards) like cat v roomba: https://github.com/raysan5/raylib-games/tree/master/cat_vs_roomba/src
- [ ] Create Doors
- [ ] Add a pause menu with textual options
- [ ] Add system for exporting resources to code for easier binary packaging ([see branch packager](https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html))
- [ ] Create Nav mesh for "walkability"
- [ ] Add some way for entities to subscribe to certain keys so we can more easily keep track of what keys are being requested over lifetime
- [ ] Fix corner walls
- [ ] Upgrade Astar to ThetaStar (worth doing?)
- [ ] support for tile sheets
- [ ] Consider using https://github.com/raysan5/rres for resources
- [ ] Particle system?
- [ ] Spritesheet animator
- [ ] consider switching to https://github.com/graphitemaster/0xABAD1DEA for all of our static globals
- [ ] Look into if its worth using fmod for sound: https://www.fmod.com/
- [ ] Investigate Fiber jobs: http://gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
- [ ] Create / Use a flatmap/flatset for better cache locality on smaller data sets
- [ ] Should we be using arena allocators?


## infra changes

- [ ] collision for player to change to cylinder
- [ ] pressing two movement at the same time while moving camera sometimes feels weird
- [ ] In pause menu, remap key bindings in layer for arrows keys to choose options
- [ ] rendering oreder for text background is weird
- [ ] likely dont need to queue network packets since we always send the full state

- [ ] AttachmentSystem (+ Attachment/Attachable): unify held item/furniture/handtruck/rope via generic parent→child attachment with local offsets/orientation/collidability; removes duplicated update math and special-cases; reduces desyncs.
- [ ] Progress/Cooldown components + ProgressRender: centralize timers/progress bars (work, patience, fishing, triggers) and a single HUD/bar renderer; fewer bespoke render paths and less UI drift.
- [ ] TriggerAreaSystem: own entrants counting, validation, progress/cooldown and activation; removes scattered trigger logic and TODOs in systems; clearer round/lobby/store flows.
- [ ] GameState change hooks (StateManager on_change): register once to handle player moves, store open/close, resets; eliminates manual cross-calls sprinkled across systems; safer transitions.
- [ ] Entity Tags/Groups (+ TaggedQuery): bitset tags like Store, Permanent, CleanupOnRoundEnd; simplifies queries and bulk operations; replaces ad‑hoc include_store_entities and type checks.
- [ ] EntityRegistry (id → shared_ptr): O(1) lookup for ids; removes linear scans in helper; safer hand-offs for systems needing shared ownership.
- [ ] CollisionCategory/CollisionFilter components: data-driven collision layers and exceptions (e.g., attached items, MopBuddy/holder, rope); replaces hardcoded branches in is_collidable; easier to reason about.
- [ ] PrefabLibrary (data-driven entity builders): move repetitive makers to JSON prefabs (like drinks/recipes already); shrinks large makers file; reduces human error and speeds iteration.
- [ ] BillboardText/WorldLabel component + system: single way to draw floating names/prices/speech bubbles/progress labels; consistent styling/sizing and fewer one-offs.
- [ ] Ability + Cooldown pattern: normalize “do work” interactions (squirter, indexer, adds_ingredient) using Progress/Cooldown; reduces bespoke timers/flags and makes balance easier.
- [ ] LifecycleSystem (deletion/cleanup policies): tag-driven bulk delete/persist on state changes; removes bespoke cleanup loops and TODOs about tagging.
- [ ] EventBus (GameEvents): small pub/sub around existing event pattern for AttachmentChanged, TriggerActivated, UpgradeUnlocked, StateChanged; decouples systems and reduces globals traffic.
- [ ] TransformFollower sockets (named anchors): define front/right/top sockets for precise child placement while following; deletes face-direction math scattered across updates.
- [ ] InputContext system: drive KeyMap by context (Menu/Game/Paused) instead of hardcoding; clarifies input paths and fixes edge-cases when menus overlap gameplay.

- [ ] vendor/afterhours: switch all includes to the local wrapper `src/ah.h` (ensures `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` and bitsery includes). Update: `src/layers/gamelayer.h`, `src/job.h`, `src/components/base_component.h`, `src/entity.h`.
- [ ] vendor/afterhours: stop including internal headers like `afterhours/src/base_component.h`; include only the public `afterhours/ah.h` via our wrapper.
- [ ] vendor/afterhours: centralize serialization on vendor implementations. Enable `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` via build flags and remove local serializers for `afterhours::Entity` and `afterhours::BaseComponent` in `src/entity.h` and `src/components/base_component.h`. Verify network roundtrips.
- [ ] vendor/afterhours: standardize on `afterhours::Entities`/`afterhours::RefEntity` aliases; remove duplicate `using` aliases in `src/entity_helper.h` and elsewhere.
- [ ] vendor/afterhours: evaluate migrating `EntityHelper` functionality (create/get/delete, range/collision queries, cleanup) to vendor equivalents to reduce duplication; replace linear scans with registry/lookup if provided.
- [ ] vendor/afterhours: compare our `EntityQuery` to vendor query APIs; if equivalent (whereHasComponent/whereInRange/orderByDist), migrate usage to vendor to shrink maintenance.
- [ ] vendor/afterhours: review job/task facilities; decide whether to adapt `src/job.h` to vendor API or keep custom; document decision.
- [ ] vendor/afterhours: check for vendor component-registration utilities for polymorphic serialization; replace manual `MyPolymorphicClasses` maintenance if available.
- [ ] vendor/afterhours: ensure submodule is initialized and pinned (`.gitmodules`); build uses `-Ivendor/` include path. Add CI guard to fail if submodule missing. Resolve nested submodules (e.g., `vendor/cereal`) by adding URLs or vendoring headers to avoid update failures.


## code health (readability, stability, and build hygiene)

- [ ] Create a precompiled header (pch.hpp) for heavyweight third-party headers (raylib/rlgl/raymath, fmt, nlohmann/json, bitsery, magic_enum, argh) to reduce compile times
- [ ] Split src/engine/graphics.h into graphics_types.h (types + declarations, no raylib includes) and graphics.cpp (definitions); move operator<< implementations out of the header
- [ ] In src/engine/model_library.h, forward declare `namespace raylib { struct Model; }` and move raylib includes and model conversion logic into a new model_library.cpp
- [ ] Introduce a lightweight network/fwd.h or network/api.h used by src/game.cpp instead of including heavy network/network.h; refactor call sites accordingly
- [ ] Run Include-What-You-Use (IWYU) across src/; add a scripts/run_iwyu.sh that consumes compile_commands.json and fix reported over-includes
- [ ] Reduce transitive includes in public headers: include only what you use, prefer forward declarations for Files, Library, Singleton where only refs/pointers are needed
- [ ] Avoid including <raymath.h> and <rlgl.h> from headers; include them only in .cpp files that need them
- [ ] Isolate template-heavy headers (bitsery, serialization) to dedicated headers included only by .cpp that need them; avoid pulling them into broadly included headers
- [ ] Add -ftime-trace to a build target to profile compile hotspots and track before/after improvements; check in a short report
- [ ] Enable sanitizers in Debug builds: ASan + UBSan by default; TSan for network tests
- [ ] Add clang-tidy with bugprone-*, clang-analyzer-*, cppcoreguidelines-*, readability-*, performance-*; wire it into CI and a local script
- [ ] Turn warnings into errors in CI and raise warning level: -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wimplicit-fallthrough
- [ ] Strengthen types: replace naked int/float parameters with enum class and strong typedefs (EntityId, PlayerId); use std::chrono for durations
- [ ] Prefer std::unique_ptr/std::shared_ptr for ownership; use gsl::not_null for non-owning pointers
- [ ] Add RAII wrappers for raylib resources (Model/Texture/Sound) with proper unload in destructors to prevent leaks and double-frees
- [ ] Use tl::expected (vendor expected.hpp) for fallible operations (file IO, network parsing) instead of bool/error out-params
- [ ] Apply [[nodiscard]] and noexcept where appropriate for public APIs to catch ignored results and enable better codegen
- [ ] Add unit tests for serialization/deserialization (e.g., ModelInfo) and basic network message round-trips; add fuzz tests for malformed inputs
- [ ] Add guard helpers for index/bounds checks and precondition asserts; replace undefined behavior with explicit errors
- [ ] Add a crash handler with backward-cpp to capture symbolized stack traces in Debug builds
- [ ] Document include policy and code style; add a pre-commit hook to run format, clang-tidy (local-only), and basic static checks
- [ ] Audit and minimize global/singleton usage; make access thread-safe or pass explicit context; wrap network:: globals with atomics or accessors


## no repro

- [ ] cosmopolitan model is invisible
- [ ] PS4 Controller touchpad causing “mouse camera rotation”<br>clicking the touchpad & analog stick in the opposite direction cam rotates that way


## design decisions

- [ ] Simple Syrup doesnt dissapear after one use and its kinda the only one that does that…
- [ ] penalty if you make too much extra? waste too much ingredients
- [ ] Text doesnt rotate based on the camera
- [ ] Not clear you can cycle through alcohols
- [ ] guys keep coming back to register. eventually need to add money system or something
- [ ] Should alcohol have to be put back? should it be like the soda / simple syrup
- [ ] add practice mode to learn recipe
- [ ] why should you clean up the bar? do people not want to come in? <br><br>cant serve until its clean?
- [ ] Should the roomba only spawn by default for single player games? Should it spawn at the beginning ever?
- [ ] warn player when they are deleting something that we need
- [ ] need to add some ui to saw how many or when more people will spawn as its not clear
- [ ] should customers be able to look like players?
- [ ] add an are you sure? when switching resolution and languages
- [ ] settings dropdown doesnt respect selected language
- [ ] - more likely to vomit if they waited longer for their drink ?
- [ ] should the day be longer based on number of customers
- [ ] - waiting reason overlaps with customer count at 1600x900 but not 1080p<br><br>- fixed but need to look into how the sizing works cause it should be proportional
- [ ] - should alcohols be multi-use and then you throw out the empty bottle
- [ ] - patience for first round should be double or triple?
- [ ] - not enough customers to need automation?


## broke

- [ ] mojito model is big square
- [ ] mai tai has no model
- [ ] having pathfinding crashes when rendering the waiting queue (disabled it for now )
- [ ] lime doesnt want to go into drink when cup is in register
- [ ] lime wont go in unless i add lime juice first?
- [ ] toilet hitbox is messed up
- [ ] - vomit hitbox is hard especially without mop

## complete

**Complete**
- [x] joining twice from a remote computer crashes the host
- [x] drop preview box sometimes has the wrong color
- [x - bug where you cant place the table next to the register<br>- => (i’ve disable bounds checking on placement for now)
- [x] Cant repro but i got the FF box to show trash icon inside. putting it back in the trash and taking out fixed it
- [x] remove job system and switch to just tons of components HasPath, CanWaitInQueue, CanIdle, CanMop, etc
- [x] add reroll to shop
- [x] need preview for where item will go
- [x] Tell the player how many customers are coming this round
- [x] hard to tell that a new machine/stockpile has been spawned in after you get an upgrade
- [x] at round 3 the people got stuck in line as if there was an invis person at the front
- [x] highlight spots on the map where this thing can go
- [x] day 3 doesnt work, i think it skips unlock screen and that breaks it
- [x] During planning its hard to know what each machine it, not obvious
- [x] default language is reverse which is confusing
- [x] vomit is broken not working
- [x] Add purchasing medicine cab
- [x] Roomba keeps getting stuck at exit
- [x] BUG: Hide pause buttons from non-host since they dont really do anything anyway
- [x] Client player cant change settings because menu::State is being overriden by host
- [x] - you can fill up the cup while its in the cupboard
- [x] We probably need some way in Progression screen to know what the new drink's recipe is like
- [x] if you take the drink back from the customer you crash
- [x] Automatically teleport new players when joining InRound / Planning etc
- [x] controls dont work for gamepad in settings during game




%% kanban:settings
```
{"kanban-plugin":"basic"}
```
%%
