# Plan: Evaluate `immer` (persistent immutable data structures) for Pub Panic

## Goal
Decide whether adopting [`arximboldi/immer`](https://github.com/arximboldi/immer) is **worth it for this codebase**, and if so, **where to use it first** with the smallest architectural disruption.

This repo is a C++20 raylib game with:
- ECS (`afterhours`) using mutable entities/components and `std::shared_ptr`
- Networking via Steam GameNetworkingSockets + `bitsery` serialization
- Save/load via snapshot serialization (`save_game::SaveGameFile { header, Map map_snapshot }`)
- Input recording/replay + replay validation specs
- A small UI state store keyed by `uuid` (`ui::StateManager`)

## What `immer` is (in 1 paragraph)
`immer` provides **persistent** (structurally shared) immutable containers like vectors/maps/sets. “Copying” a modified value is cheap because it reuses most of the old structure. This can make it easier to build **value-oriented** architectures with cheap snapshots, straightforward undo/redo, and safe multi-reader concurrency.

## Why this might matter *here*
This codebase already has multiple features that benefit from cheap snapshots and value comparisons:
- **Replay + validation**: input replay exists; validation asserts world conditions at end of replay. Persistent snapshots can enable stronger determinism checks and debugging.
- **Networking**: `Map` is serialized for clients; if we ever want **delta replication** or cheap change detection, persistent structures can help (but may be a large refactor).
- **Save/load**: snapshotting is the current approach; persistent structures could make incremental saves, “quicksave ring buffers”, or rollbacks cheaper.
- **UI state**: `ui::StateManager` is a mutable `std::map<uuid, shared_ptr<UIState>>`. If UI bugs benefit from time-travel debugging, persistent maps are a natural fit.

## Key constraint: the ECS is mutable and pointer-heavy
Core world state uses:
- mutable components
- pointer-linked serialization (`bitsery::ext::PointerLinkingContext`, `StdSmartPtr`, etc.)

`immer` pays off most when:
- values are immutable (or treated as immutable)
- updates are expressed as “old value -> new value”
- structural sharing is meaningful across frames

It is **not** an obvious win to drop `immer::vector` into hot mutable ECS storage, or to store `std::shared_ptr<Entity>` inside persistent containers and expect big gains.

## Hypotheses (what we’re trying to prove/disprove)
- **H1 (UI/debug)**: Persistent UI state snapshots would make UI issues easier to debug and would enable undo/redo for UI interactions with modest cost.
- **H2 (replay)**: Capturing periodic immutable “checkpoints” of *selected* game state would make replay validation stronger and failure diagnosis faster, without expensive deep copies.
- **H3 (network)**: A value-oriented “replicated state” layer (separate from ECS) could reduce bandwidth/CPU via diffing, but only if we’re willing to add/maintain that layer.
- **H4 (mapgen)**: If we ever add backtracking to WFC/layout generation, persistent grids/sets could simplify implementation and debugging.

## Candidate adoption targets (ranked)

### 1) UI state store (`src/engine/ui/state.h`)
**Why it’s a good first target**
- Small surface area, minimal coupling to ECS/network serialization.
- Natural value semantics: “state at frame N” vs “state at frame N+1”.
- Enables features like undo/redo, time-travel debugging, and easy diffing.

**What “success” looks like**
- A proof-of-concept where a UI interaction produces a new state value.
- Ability to keep a short history (e.g., last 60 frames) cheaply.
- No meaningful perf regression; manageable compile times.

### 2) Replay checkpoints (selected data, not the whole ECS)
**Why**
- Input replay exists; adding a *small* deterministic snapshot can make failures actionable.

**What to snapshot**
- Prefer “compact, pure-ish” data already used in validations (e.g., day count, progression counters, seed, player positions/inventories) rather than entire `Map`/ECS graphs.

**What “success” looks like**
- Faster diagnosis when a replay diverges: compare checkpoint hashes or compare value structures directly.

### 3) A separate “replicated model” for networking (high effort, high potential payoff)
**Reality check**
- Today, `Map`/`LevelInfo` are serialized via `bitsery` and include pointer graphs.
- Moving to `immer` in the core world state would be a major architectural shift.

**Lower-risk alternative**
- Keep ECS mutable; introduce a derived immutable “replication view” struct built each net tick (or at lower rate), and diff it.
- `immer` might help here, but only after proving that diff-based replication is a desired direction.

### 4) Map generation (optional)
WFC currently does not backtrack. If backtracking is added later, persistent state could help. This is likely not the best *first* adoption unless mapgen is currently a pain point.

## Integration options (no code changes in this doc; choose later)
`immer` is header-only and would fit the current build setup well.

- **Vendoring approach (likely simplest here)**:
  - Place the upstream `immer/` folder under `vendor/immer/` so includes work as `#include <immer/vector.hpp>` via existing `-Ivendor/`.
  - Works for Linux makefile and the Windows VS project (already includes `..\vendor\`).
- **vcpkg**:
  - Possible, but this repo doesn’t appear to be using vcpkg on Linux today.
- **CMake install**:
  - Not aligned with the current makefile-based build.

## Serialization boundary plan (important)
If `immer` containers cross boundaries that use `bitsery` or network packets:
- **Option A (preferred early)**: keep `immer` structures internal; convert to/from `std::vector`/`std::map` at boundaries.
- **Option B**: implement `bitsery` adapters for the subset of `immer` containers you use (more work, but can be clean).

Decision rule: do not invest in serialization adapters until a pilot shows real value.

## Evaluation work items (spikes) and outputs

### Spike 0: License + maintenance check
- **Check** `immer` license and compatibility with this project’s distribution.
- **Check** how actively maintained it is (recent releases, CI, issues).
- **Output**: short note: “OK to vendor”, plus version pin policy.

### Spike 1: Build/compile impact (baseline)
- Measure compile time impact when including `immer` in one translation unit.
- Watch for template bloat and precompiled header interactions.
- **Output**: compile time comparison, binary size delta (rough).

### Spike 2: UI state pilot (recommended first real prototype)
- Replace (or wrap) `ui::StateManager::states` with a persistent map in a small experimental layer.
- Add ability to keep N historical versions and switch between them for debugging.
- **Output**: demo steps + perf/complexity notes + recommendation.

### Spike 3: Replay checkpoint pilot (selected-state snapshot)
- Define a minimal “ReplayCheckpoint” value struct.
- Produce checkpoints periodically; compare with expected results or between runs.
- **Output**: does it catch divergence earlier? does it help debugging?

### Spike 4: Network replication view (only if we want delta replication)
- Define a small immutable replicated view (e.g., players + key world counters).
- Diff view between ticks; measure bandwidth and CPU.
- **Output**: measured savings vs complexity; go/no-go for expanding.

## Go / No-Go criteria

### Go if (any of these)
- **Debuggability**: time-travel / undo / replay diagnosis improves significantly with modest code complexity.
- **Performance**: measurable reduction in copying or diffing cost for a target subsystem.
- **Correctness**: fewer “heisenbugs” from shared mutable state; simpler concurrency boundaries.

### No-Go if (any of these)
- **Compile-time cost** is disruptive (especially with PCH) without strong benefits.
- **Memory overhead** is too high for target platforms.
- **Integration friction** (serialization, tooling, debugging) outweighs gains.
- **Refactor scope** inevitably pushes into ECS core and pointer graphs to realize value.

## Risks / gotchas to watch
- **Template compile times** and error verbosity.
- **Allocator / memory behavior**: structural sharing can increase retained memory if old versions are kept too long.
- **False sense of immutability**: storing mutable pointers inside persistent containers can undermine the model.
- **Serialization mismatch**: pointer-graph serialization and persistent values don’t naturally align.

## Alternatives (if we decide “no”)
- Purpose-built undo/redo stacks for specific features (cheaper than general persistence).
- Hash-based change detection on serialized blobs for replay/network debugging.
- More targeted container swaps (`ankerl::unordered_dense`, flat maps, etc.) for performance without a paradigm shift.
- If value-oriented architecture is the goal, evaluate `lager` (mentioned by `immer` authors) as a higher-level pattern reference, without adopting a new core container library immediately.

## Recommendation (based on current repo scan)
- **Most plausible near-term value**: UI state history/time-travel + replay checkpoints (both can be done without touching ECS internals).
- **Highest-risk / highest-effort**: using `immer` for core world/ECS state or as a direct replacement for current pointer-heavy serialized structures.

If you want a single first bet: **prototype persistent UI state** and judge the developer-experience and performance impact before considering anything deeper.

## Afterhours ECS check (vendor submodule)
I pulled `vendor/afterhours` (it’s a git submodule) and skimmed the core ECS + its immediate-mode UI plugin.

### What afterhours is doing today
- **Entity storage**: `EntityHelper` stores `Entities` as `std::vector<std::shared_ptr<Entity>>`, plus a `temp_entities` vector that gets merged after systems run.
- **Component storage**: each `Entity` owns an `std::array<std::unique_ptr<BaseComponent>, max_num_components>` and a bitset to track presence.
- **System iteration**: systems iterate the entity vector and call `entity.get<T>()` / `entity.addComponent<T>()` mutating in place.
- **Queries**: `EntityQuery` builds a temporary `RefEntities` vector by scanning all entities, then filters via `std::partition`.
- **UI (in afterhours)**: the “immediate mode” UI keeps a global `std::map<UI_UUID, EntityID> existing_ui_elements` to reuse UI entities across frames.

### Would `immer` help inside afterhours?
**Mostly no**, for the same reason it didn’t look compelling for the main game ECS:
- The ECS is **fundamentally mutable**, pointer-backed, and optimized around “update in place.” Persistent containers shine when the *data itself* is treated as immutable and updates produce new values.
- Swapping `std::vector`/`std::map` for `immer` containers inside the ECS would add template/allocator complexity and likely **increase overhead** without enabling the main benefits (cheap snapshots and safe multi-reader concurrency), because:
  - components are still `unique_ptr` to mutable objects
  - entity identity is pointer/ID based and frequently mutated

### Narrow cases where `immer` could matter (only if you want these features)
- **Lock-free “publish/subscribe” snapshots** of small metadata (e.g., UI element registry map, a debug overlay model, or a read-only “view” of some derived state) by swapping whole persistent values.
- **Editor-style undo/redo / time-travel** for UI/layout configuration, *if* afterhours UI state is refactored to be value-oriented rather than stored in ECS components.
- **Rollback** would require far more than containers: you’d need component values to be immutable (or copy-on-write) and a deterministic update model.

### Practical conclusion for this repo
If you already feel “it doesn’t seem worth it,” the afterhours internals reinforce that: **adopting `immer` inside the ECS is unlikely to pay off** unless you’re explicitly pursuing a value-oriented architecture (time-travel/undo/rollback/concurrent-read snapshots) and are willing to reshape how component data is represented.

