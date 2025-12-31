# Vendor Afterhours migration plan (new setup)

This document reviews what’s new in `vendor/afterhours` (the Afterhours submodule) and lays out a phased plan to migrate PharmaSea to the updated setup **with clear “what” and “why”**.

**Baseline assumption for this plan**: the `vendor/afterhours` submodule is pinned to:
- branch: `cursor/remove-serializing-pointers-438b`
- feature set: handle refs + pointer-free policy + snapshot take/apply + pooled components

---

## What’s new in `vendor/afterhours` (relevant to PharmaSea)

- **Single-include entrypoint**
  - `vendor/afterhours/ah.h` now includes `src/ecs.h` (recommended “one include” for core ECS).
  - `PLUGIN_API.md` recommends plugins include `ecs.h` rather than individual headers.

- **Formal plugin/public API boundary**
  - `vendor/afterhours/PLUGIN_API.md` explicitly documents what plugins are allowed to include and which `EntityHelper` methods are “public” vs forbidden internals.
  - This is important for us because our codebase has historically mixed “engine/game code” and “afterhours-style plugin code”.

- **Handle-based entity referencing + pointer-free snapshot surfaces**
  - New core types/APIs:
    - `afterhours::EntityHandle` (`src/core/entity_handle.h`)
    - `afterhours::OptEntityHandle` (`src/core/opt_entity_handle.h`)
    - `afterhours::snapshot::*` helpers (`src/core/snapshot.h`)
    - pointer-free guardrails (`src/core/pointer_policy.h`)
  - The direction is: **store handles/IDs in persisted state, resolve via `EntityHelper` at runtime**.

- **Snapshot “apply” helpers (Phase 6)**
  - Afterhours now includes an “apply snapshot” workflow in `src/core/snapshot.h` plus documentation (`SNAPSHOT_APPLY.md`).
  - This is explicitly designed for **rebuilding world state** from pointer-free snapshots and handling missing entities with a defined policy.

- **Pooled component storage (Phase 4)**
  - Afterhours has introduced pooled component storage (`src/core/component_pool.h`, `src/core/component_store.h`) and refactored `Entity` to use it.
  - Impact: anything that tries to serialize/inspect `Entity` internals (e.g., per-entity component pointer arrays) is expected to break and should be replaced by the snapshot surface.

- **EntityQuery performance improvements**
  - `EntityQuery` has recent work for short-circuiting/caching (important once “resolve by handle” becomes common and we rely more heavily on queries).

- **Policy enforcement + regression infrastructure**
  - Afterhours now includes:
    - compile-time pointer-free checks via `pointer_policy.h`
    - regression examples for snapshots (`example/snapshot_pointer_free_regression/`)
    - “compile-fail” style guard scripts (e.g. `check_compile_fail_tests.sh`) to ensure pointer-like types don’t leak into snapshot surfaces.

- **More built-in plugins**
  - `vendor/afterhours/src/plugins/` includes reusable building blocks we currently maintain ourselves (or have planned to extract), e.g. `files`, `settings`, `timer`, `pathfinding`, `translation`, `sound_system`, `ui`, `animation`, etc.

---

## Current state in PharmaSea (what this affects)

- **Afterhours is a submodule, but PharmaSea wraps it**
  - We have a wrapper header `src/ah.h` that sets macros and includes Afterhours headers.
  - We also have *project-local* wrappers/adapters:
    - `src/entity_helper.h` (PharmaSea `EntityHelper`)
    - `src/entity_query.h` (`EQ` extending `afterhours::EntityQuery`)

- **Serialization is not pointer-free today**
  - We currently serialize:
    - `Entity` component storage via smart pointers (see `src/entity.h` bitsery support)
    - `std::shared_ptr<Entity>` (directly)
    - and we still carry pointer-linking infrastructure in network/save contexts (see `docs/pointer_free_serialization_options.md`)
  - This conflicts with the Afterhours “pointer-free snapshot surface” direction.
  - **Important update with the new Afterhours branch**: Afterhours now uses **pooled component storage**, so our current `serialize(Entity&)` logic that walks per-entity component pointer arrays is not a stable integration point. We should treat “serialize Entity directly” as deprecated/unsupported and migrate to snapshots.

- **Afterhours systems are already being adopted**
  - We’ve started migrating “old system functions” into Afterhours systems (see `src/system/afterhours_*` and `SystemManager::systems.tick(...)`).

---

## Migration goals (why we’re doing this)

- **Unblock submodule upgrades**: reduce local forks and align with Afterhours’ documented public APIs.
- **Enable pointer-free save/network formats**: move persisted references to handles/IDs (no raw/smart pointer graphs).
- **Improve correctness under churn**: stale references should fail safely (generation mismatch) instead of becoming UB.
- **Keep iteration fast**: preserve dense per-frame iteration while allowing stable addressing.
- **Adopt reusable plugins where it reduces maintenance** (optional, but a major payoff over time).

---

## Phased plan

### Phase 0 — Pin, bootstrap, and verify the new Afterhours baseline (no gameplay changes)

**What**
- Ensure `vendor/afterhours` is **always initialized** in dev + CI (`git submodule update --init --recursive`).
- Ensure the submodule is on the intended branch: `cursor/remove-serializing-pointers-438b`.
- Add a lightweight CI verification step that runs Afterhours’ own fast checks/examples (at minimum, its non-raylib examples/tests), so we know we’re not pinning to a broken snapshot/pool build.
- Inventory all current integration points:
  - Includes: where we include `afterhours/*` vs project wrappers.
  - Plugin usage: which Afterhours plugins we already rely on (directly or indirectly).
  - Serialization: every place we serialize `Entity*`, `RefEntity`, `OptEntity`, `shared_ptr<Entity>`, or use Bitsery pointer contexts.
- Identify which gameplay systems store long-lived entity references (highest-risk migration surface).

**Why**
- This prevents “surprise” breakage and lets us scope the real work: **API migration** vs **data-model migration** vs **serialization migration**.

**Deliverables**
- A checklist of reference/serialization sites, with owners and suggested replacement types (`EntityHandle`/`OptEntityHandle`/IDs).
- CI script/docs updated so submodule is reliably present.

---

### Phase 1 — Integrate the new Afterhours setup cleanly (low risk)

**What**
- Align includes with Afterhours’ recommended entrypoints:
  - Prefer including `afterhours/ah.h` (submodule entrypoint) and/or `afterhours/src/ecs.h` where appropriate.
  - Avoid reaching into Afterhours internals that violate `PLUGIN_API.md` (treat it as the compatibility contract).
- Decide how we want to handle **`expected`** and logging/validate overrides:
  - Afterhours ships `expected.hpp` and supports `AFTER_HOURS_REPLACE_LOGGING` / `AFTER_HOURS_REPLACE_VALIDATE`.
  - PharmaSea currently has wrapper behavior in `src/ah.h` that may be carrying legacy compatibility hacks.
  - Note: this branch has recent refactors around `expected.hpp` namespacing/fallbacks; keep our wrapper minimal and avoid redefining expected types unless we must.

**Why**
- This reduces “mystery include” problems and makes future Afterhours updates less painful.

**Deliverables**
- A clear include/macro policy documented in-repo (what we define, and why).
- A build that compiles cleanly with the submodule present.

---

### Phase 2 — Adopt Afterhours’ persisted reference model (handles + OptEntityHandle) (medium risk, incremental)

**What**
- Adopt Afterhours’ handle types for **persisted or long-lived relationships**:
  - Use `afterhours::EntityHandle` for durable references.
  - Use `afterhours::OptEntityHandle` as the standard “persisted reference helper” where we need fallback behavior (e.g., temp entities pre-merge or transitional code).
- Keep “short-lived” query ergonomics:
  - It’s fine for queries/systems to work with `Entity&`, `RefEntity`, `OptEntity` **as transient values**.
  - The rule is: **don’t store those in long-lived component state**.
- Update PharmaSea components/systems that currently store:
  - `Entity* parent`
  - `RefEntity`/`OptEntity` as member state
  - `std::shared_ptr<Entity>` relationship fields (e.g. “held item” style patterns)
  - …to store handles instead and resolve on use.

**Why**
- Handles are the bridge that lets us:
  - remove pointer-based persistence,
  - keep runtime lookups fast,
  - and make stale references safe.

**Deliverables**
- A running game where the majority of cross-entity relationships are `EntityHandle`/`OptEntityHandle`.
- Debug logging/metrics for “failed handle resolves” (to catch missing-fixup bugs early).

---

### Phase 3 — Switch persistence + networking to Afterhours snapshots (take/apply) (highest payoff)

**What**
- Stop serializing `afterhours::Entity` and `shared_ptr<Entity>` graphs directly in PharmaSea.
- Reframe save/network state as **explicit, pointer-free DTO snapshots** using Afterhours’ snapshot APIs:
  - `afterhours::snapshot::take_entities()`
  - `afterhours::snapshot::take_components<Component, DTO>(converter)`
  - enforce pointer-free DTOs via `pointer_policy.h` (and mirror Afterhours’ “compile-fail regression” approach for our project-level snapshot DTOs).
- Use Afterhours’ **apply** helpers to rebuild state:
  - adopt a clear `MissingEntityPolicy` decision per domain (save-load vs network reconciliation), rather than relying on pointer graph “best effort”.
- Remove Bitsery pointer-linking context **once no pointer-like data remains** in the format.
- Add versioning/migration strategy for existing save files (format bump + one-time upgrade path or incompatibility policy).

**Why**
- This is the core “new setup” migration: Afterhours is moving toward **explicit snapshot surfaces** specifically to avoid pointer serialization.
- This also improves determinism and reduces subtle deserialization bugs (pointer graphs, ordering, address reuse).

**Deliverables**
- Save/load working with **zero pointer values in serialized data**.
- Network sync updated (or clearly split) to use pointer-free packet payloads.
- A regression test suite for snapshot round-trips + apply semantics (at minimum: churn + handle validity + missing-entity policy behavior).

---

### Phase 4 — Leverage Afterhours performance/correctness upgrades (pooled components + query improvements)

**What**
- Treat Afterhours’ pooled component storage as a **feature we benefit from by not fighting it**:
  - remove/avoid any code that assumes entity component internals are pointer arrays
  - keep all component access through `Entity::addComponent/get/has/remove` and query APIs
- Explicitly validate that our “hot query patterns” align with Afterhours’ updated `EntityQuery` behavior (short-circuiting/caching) and avoid accidental double work (e.g., `has_values()` followed by `gen_first()` patterns in tight loops).

**Why**
- This branch is not just “serialization plumbing”; it includes structural ECS changes meant to keep the pointer-free model performant.

**Deliverables**
- No remaining PharmaSea code depends on Afterhours `Entity` internal layout.
- Confirmed query patterns in hot paths are efficient and match new semantics.

---

### Phase 5 — Plugin adoption + de-duplication (optional, but strategic)

**What**
- Evaluate adopting Afterhours plugins where we currently have PharmaSea-only equivalents:
  - `files` (paths/resource iteration)
  - `settings`
  - `timer`
  - `pathfinding`
  - `translation` (note: this may not map 1:1 to our `.po` system)
  - `sound_system` / audio utilities
  - `ui` / layout helpers / animation
- For each candidate plugin:
  - Identify what we would delete/replace in PharmaSea,
  - define a compatibility shim layer (if needed),
  - migrate one subsystem at a time behind a feature flag.

**Why**
- This reduces long-term maintenance and makes Afterhours upgrades deliver real value (bug fixes/features land in the shared library).

**Deliverables**
- A prioritized plugin adoption list with “migrate” vs “keep local” decisions and rationale.

---

### Phase 6 — Validation, performance, and rollout hardening

**What**
- Add focused tests and checks:
  - handle lifecycle: create → resolve → delete → stale handle fails → slot reuse increments generation
  - snapshot correctness: pointer-free DTO assertions + round-trip tests
  - snapshot apply correctness: missing-entity policies behave as intended (drop vs keep vs error)
  - gameplay smoke tests for the highest-churn systems (inventory/holding, AI targets, UI attachments, etc.)
- Performance validation:
  - confirm hot paths are O(1) resolve (no linear scans for repeated lookups)
  - confirm per-frame iteration remains dense (no “iterate sparse slot table” regressions)
- Rollout strategy:
  - land changes behind compile-time flags or runtime feature toggles where needed,
  - keep save format migration explicit and testable.

**Why**
- Migration work is only “done” when it’s stable under gameplay churn and does not regress frame-time.

**Deliverables**
- A stable “migration complete” build with documented flags and a measured performance baseline.

---

## Key risks + mitigations

- **Risk: conflicting vendor dependencies (magic_enum, sago/platform_folders, etc.)**
  - **Mitigation**: standardize include paths and prefer one copy (project vendor vs Afterhours vendor), leveraging Afterhours’ `__has_include` fallbacks.

- **Risk: hidden reliance on pointer identity**
  - **Mitigation**: introduce handles first (Phase 2) while still running with current gameplay, then migrate serialization (Phase 3) once usage is clean.

- **Risk: save file compatibility**
  - **Mitigation**: explicit version bump + documented upgrade policy; add round-trip tests early.

---

## Definition of done

- **Build & tooling**
  - `vendor/afterhours` is reliably present in CI and local dev.
  - Code adheres to Afterhours’ public API boundary where applicable (`PLUGIN_API.md`).

- **Runtime model**
  - Long-lived entity relationships are stored as `EntityHandle`/`OptEntityHandle` (not pointers/refs).

- **Persistence/network**
  - Save-game and network payloads contain **no raw pointers or smart pointers**.
  - Snapshot/DTO surfaces enforce pointer-free policy (compile-time checks).

