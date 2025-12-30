# Afterhours: migrating to a handle-based entity store (slots + generation + dense iteration)

This doc describes what it would take to implement the **stable-slot handle system** (slot index + generation, with a dense live list for iteration) **inside `vendor/afterhours`**, and which parts would be **API-breaking** vs what can be done **without breaking existing public APIs**.

---

## Current Afterhours APIs relevant to this change

### Entity identity & reference types
In `vendor/afterhours/src/core/entity.h`:

- `using EntityID = int;` and `Entity::id` is assigned from a global atomic increment.
- `using RefEntity = std::reference_wrapper<Entity>;`
- `struct OptEntity` is a wrapper over `std::optional<std::reference_wrapper<Entity>>`.

These are *non-owning* references. They are convenient, but they can become invalid if an entity is deleted (because they store a direct reference).

### Entity storage & iteration
In `vendor/afterhours/src/core/entity_helper.h`:

- `using Entities = std::vector<std::shared_ptr<Entity>>;`
- `EntityHelper` stores:
  - `Entities entities_DO_NOT_USE;` (main list)
  - `Entities temp_entities;` (new entities created this frame)
  - `merge_entity_arrays()` moves `temp_entities` into the main list
  - `cleanup()` removes entities marked `cleanup` via `erase/remove_if` (this compacts the vector)
- `EntityHelper::getEntityForID` is a linear scan over `get_entities()`.
- `EntityHelper::forEachEntity` loops over `get_entities()`.

In `vendor/afterhours/src/core/entity_query.h`:

- `EntityQuery` takes a snapshot of the `Entities` vector (copies it) and then builds `RefEntities` by pushing references to every `Entity` in that vector.
- It already expects the `Entities` container to be **dense** (it iterates a contiguous vector).

### Public API boundary (important for “breaking” analysis)
`vendor/afterhours/PLUGIN_API.md` defines the plugin/public boundary:

- “Allowed” access is primarily through `EntityHelper` static methods and `EntityQuery`.
- It explicitly calls out that direct access to `entities_DO_NOT_USE`, `temp_entities`, etc. is **forbidden**, even though the struct members are currently public in the header.

This distinction matters:
- If downstream code follows `PLUGIN_API.md`, many internal refactors can be done with **no source-breaking** changes.
- If downstream code reaches into `EntityHelper` fields directly, any refactor is likely **source-breaking**.

---

## Target design inside Afterhours

### Requirements we’re trying to satisfy
- **Fast per-frame iteration** over all live entities (dense, cache-friendly).
- **Fast reference resolution** (O(1), no `unordered_map`, no linear scans).
- **High churn friendly** (create/delete often, without invalidating references silently).
- **Pointer-free serialization friendliness** (handles are simple POD values).

### Core idea: stable slots + free list + dense live list
Inside Afterhours, introduce:

- `struct EntityHandle { uint32_t slot; uint32_t gen; };`
- `slots[]` (stable index space, reused via `free_list`)
- `dense[]` (compact list of live `slot` indices; used for iteration)
- Each slot stores:
  - owning pointer to entity (`std::shared_ptr<Entity>` initially for compatibility)
  - `gen`
  - `dense_index` back-pointer

With this, you get:
- lookup: `O(1)` slot indexing + generation check
- delete: `O(1)` swap-remove in `dense[]` + bump generation + recycle slot
- iteration: `for slot : dense` (compact, cache-friendly)

---

## What changes are needed in Afterhours (compatibility-first plan)

This plan aims to keep existing public APIs working while adding the handle system as a new capability.

### 1) Add new types (non-breaking)
- Add `EntityHandle` (POD) in a public header (likely `core/entity.h`).
- Add helper APIs on `EntityHandle`: `valid()`, equality, etc.

This is additive and should not break existing code.

### 2) Add resolve APIs (non-breaking)
Add methods on `EntityHelper` such as:
- `static EntityHandle handle_of(const Entity&);`
- `static OptEntity resolve(EntityHandle);`
- `static Entity* resolve_ptr(EntityHandle);` (optional, for perf/hot paths)

These are also additive.

### 3) Change internal storage of EntityHelper (potentially non-breaking if done carefully)
Key functions to rework:
- `createEntityWithOptions` (allocate a slot + append to dense)
- `merge_entity_arrays` (decide how temp creation interacts with dense; see below)
- `cleanup` / `delete_all_entities` (must remove from dense + invalidate slots)
- `getEntityForID` (can remain but becomes secondary; handle resolve is primary)
- `forEachEntity` (should iterate via dense)

**Compatibility trick**: keep returning a dense `Entities` vector for `EntityQuery`.

There are two workable approaches:

#### Approach 3A — Keep `Entities entities_DO_NOT_USE` as the dense list (lowest risk)
- Continue maintaining `entities_DO_NOT_USE` as the canonical dense `std::vector<std::shared_ptr<Entity>>`.
- Maintain slot bookkeeping in parallel:
  - each slot knows which index it lives at in `entities_DO_NOT_USE` (dense_index)
  - deletions swap-remove from `entities_DO_NOT_USE` and update the moved slot’s dense_index
- Result: `EntityQuery` can remain unchanged (still consumes `Entities`).

This is the easiest way to avoid breaking `EntityQuery` and `get_entities()` return type.

#### Approach 3B — Make `dense[]` be slot indices, and synthesize a view (more invasive)
- Store only `dense_slots[]` internally.
- Provide a “view” to queries/iterators.

This is more likely to be API-breaking because:
- `EntityHelper::get_entities()` currently returns `const Entities&` (a concrete vector type).
- A view/adaptor would not be the same type.

So for “no break” goals, prefer 3A.

### 4) Keep temp entity semantics (likely non-breaking, but needs design)
Afterhours currently has:
- `temp_entities` populated by `createEntityWithOptions`
- `merge_entity_arrays` merges them into main
- `EntityQuery` warns when `temp_entities` is non-empty unless forced

With handle storage, you can either:
- keep `temp_entities` as-is and only slot-register entities when merged, **or**
- keep creating into temp but also register slots immediately, and decide what “query should miss temp” means.

To avoid breaking behavior:
- keep the temp mechanism and the query warning behavior intact, at least initially.

---

## What would be API-breaking in Afterhours (and why)

### Breaking: changing `RefEntity` from `reference_wrapper<Entity>` to a handle-aware type
Today:
- `RefEntity` is an alias: `std::reference_wrapper<Entity>`.
- Call sites rely on:
  - implicit conversion between `Entity&` and `RefEntity`
  - `std::vector<RefEntity>` being trivially cheap
  - immediate dereference semantics with no failure mode

If `RefEntity` becomes a handle-based wrapper (e.g. stores `EntityHandle` and resolves on demand), then:
- conversions and overload resolution change
- `RefEntity::get()` can fail (stale handle), so APIs must return `OptEntity`-like results or assert
- existing code that assumes `RefEntity` is always valid becomes unsafe

This is source-breaking for most user code (including PharmaSea) unless introduced as a new type (e.g. `HandleEntityRef`) and migrated gradually.

### Breaking: changing `OptEntity` representation and operators
`OptEntity` currently behaves like:
- `optional<reference_wrapper<Entity>>` with `operator->`, `operator*`, `asE()`, and implicit conversions to `RefEntity`.

If you make it handle-based, you either:
- make those operators potentially fail (return null), or
- keep them but introduce runtime asserts when stale.

Either way, semantics change, and code that uses `OptEntity` as “cheap optional reference” may break or start asserting.

### Breaking: changing `EntityHelper::get_entities()` return type
`EntityHelper::get_entities()` currently returns `const Entities&` where:
- `Entities = std::vector<std::shared_ptr<Entity>>`

If you replace that with:
- a view/adaptor type,
- a `std::span`, or
- an iterator range,

then anything that expects a `std::vector<shared_ptr<...>>` reference will break (including `EntityQuery`’s current constructor path).

### Breaking (in practice): changing/removing public fields on `EntityHelper`
Even though `PLUGIN_API.md` forbids it, `EntityHelper` currently exposes members publicly in the header, including:
- `entities_DO_NOT_USE`
- `temp_entities`
- `permanant_ids`
- `singletonMap`

Any downstream code that accessed these fields directly will fail to compile if you:
- rename them
- change their types
- make them private

So: “non-breaking” assumes consumers follow the plugin/public API boundary; otherwise it’s source-breaking.

### Potential binary compatibility impact (ABI)
Afterhours is largely header-based, but if anyone links a prebuilt binary that depends on the struct layout of `EntityHelper` or `Entity`, changing their member layout is ABI-breaking. In normal use, downstream projects recompile everything, so this is usually acceptable.

---

## What can be done without breaking the public API (recommended)

The safest path is to add handles and slot bookkeeping while preserving the existing externally visible container types and query behavior.

### Non-breaking changes (assuming consumers follow `PLUGIN_API.md`)
- **Add `EntityHandle`** as a new type.
- **Add `EntityHelper::handle_of(entity)` and `EntityHelper::resolve(handle)`**.
- **Implement slot+generation bookkeeping internally**, but keep:
  - `Entities` as `std::vector<std::shared_ptr<Entity>>`
  - `EntityHelper::get_entities()` returning a dense `Entities&`
  - `EntityQuery` iterating that dense vector as it does today
- **Keep `RefEntity`/`OptEntity` unchanged** for short-lived query results.
- **Deprecate** `getEntityForID` for “reference” use (it can remain for debug/tools).

### The key compatibility technique (why it works)
Maintain `entities_DO_NOT_USE` as the canonical **dense list of live entities**, and treat the slot pool as an additional index:
- slot → (shared_ptr, gen, dense_index)
- dense_index → shared_ptr in `entities_DO_NOT_USE`

Deletion becomes:
- swap-remove from `entities_DO_NOT_USE`
- update the moved slot’s `dense_index`
- clear slot + bump generation + recycle slot

From the outside, `get_entities()` still looks like “a dense vector of shared_ptr”, so queries and plugins keep working.

---

## What you still need to change in PharmaSea even if Afterhours supports handles

Even with an Afterhours-native handle store, PharmaSea would still need to:
- stop serializing pointers (`RefEntity`, `Entity* parent`, `shared_ptr<Entity>` relationship fields)
- store relationships as handles (or IDs) instead of pointers

Afterhours supporting handles just makes that migration cleaner (you can use Afterhours’ handle type and resolver rather than having a parallel system in PharmaSea).

**Current direction (confirmed)**:
- PharmaSea will migrate **long-lived gameplay relationships** to `EntityHandle`.
- `EntityID` remains for compatibility (and possibly networking/diagnostics), but should not be relied on for hot-path relationship resolution once the migration is complete.

---

## Suggested rollout strategy for Afterhours (minimize breakage)

1) **Add `EntityHandle` + resolver APIs** (additive)
2) **Internally implement slot bookkeeping** while keeping `entities_DO_NOT_USE` dense and public methods stable (compat-first)
3) Add an opt-in query mode (new APIs) that returns handles if you want long-lived references without dangling refs
4) Only in a major version bump: consider changing `RefEntity`/`OptEntity` semantics (API-breaking) if desired

---

## Make breaking pieces opt-in (so existing users can upgrade safely)

The goal: an existing Afterhours user should be able to:
- bump the submodule / dependency version,
- rebuild,
- and have everything work **without changing their code**.

Then, when they’re ready, they can opt into the new semantics incrementally.

### Principle: additive-first, switches second, defaults last
- **Additive-first**: new types/APIs exist alongside old ones.
- **Switches second**: provide compile-time flags to opt into new behavior per-project.
- **Defaults last**: only after most users have migrated (and likely in a major version) consider flipping defaults.

### Opt-in mechanisms (recommended)

#### 1) Feature macro to enable handle-based refs (keeps old default)
Provide a macro like:
- `AFTER_HOURS_USE_HANDLE_REFS`

Behavior:
- Default (macro off): `RefEntity`/`OptEntity` remain reference-wrapper based (current behavior).
- Opt-in (macro on): expose handle-based reference types and APIs, and optionally alias `RefEntity`/`OptEntity` to them (see “strict vs soft opt-in” below).

This mirrors existing Afterhours patterns (it already uses compile-time flags like `AFTER_HOURS_DEBUG`, `AFTER_HOURS_MAX_COMPONENTS`, etc.).

#### 2) Parallel types: keep `RefEntity` stable; add new handle-native types
Even better for compatibility is to **not** change `RefEntity` at all initially.

Add new types such as:
- `struct HandleRefEntity { EntityHandle h; ... };`  (resolves to `Entity*` or `OptEntity`)
- `struct OptHandleEntity { std::optional<EntityHandle> h; ... };`

And add parallel query APIs:
- `EntityQuery::gen()` (existing): returns `std::vector<RefEntity>`
- `EntityQuery::gen_handles()` (new): returns `std::vector<EntityHandle>`
- `EntityQuery::gen_handle_refs()` (new): returns `std::vector<HandleRefEntity>` (optional)

This is the least risky way to let users migrate “leaf-by-leaf”:
- systems can still use `Entity&` locally
- long-lived relationships can store handles

#### 3) Strict vs soft opt-in (two modes)
If you do want `RefEntity` to eventually become handle-based, do it in two steps:

- **Soft opt-in (recommended early)**
  - `RefEntity` stays as-is.
  - Handle-based refs are new types.
  - Users migrate by changing their own stored fields and a few call sites.

- **Strict opt-in (for users who want full adoption)**
  - Under `AFTER_HOURS_USE_HANDLE_REFS`, you can:
    - `using RefEntity = HandleRefEntity;`
    - `using OptEntity = OptHandleEntity;`
  - This will surface compile errors immediately where code assumes reference-wrapper semantics.
  - It’s still opt-in (so existing users are unaffected).

#### 4) Versioned namespace (optional, but clean for big transitions)
Another approach (less common in game libs, but very explicit) is to ship:
- `namespace afterhours::v1` (current semantics)
- `namespace afterhours::v2` (handle-based refs)

And keep `namespace afterhours` aliased to v1 by default:
- `namespace afterhours = afterhours::v1;`

Projects can opt into v2 by defining:
- `AFTER_HOURS_API_VERSION=2` (or similar) before including headers.

This avoids macro-magic type aliasing surprises, but increases surface area.

### What should remain stable by default (to avoid forced migrations)
To ensure “pull new version and build as normal”:
- Keep `RefEntity` and `OptEntity` semantics unchanged by default.
- Keep `EntityHelper::get_entities()` returning `const Entities&` by default.
- Keep `EntityQuery` working unchanged by default.

All handle-related additions should be additive, and any aliasing of existing names should be behind an explicit flag.

### Deprecation guidance (how to encourage migration without breaking)
Once handles exist:
- Mark `getEntityForID` as “debug/tools” (or keep it, but document it as slow / not for hot paths).
- Add docs and examples showing:
  - query returns refs for short-lived use
  - persistent relationships store handles

Then, only when the ecosystem is ready:
- introduce a new major version or opt-in strict mode to make handle-based refs the default for new projects.

---

## Intern-facing implementation phases (step-by-step)

This section is written as a practical checklist for implementing the change **in Afterhours** while keeping existing users unbroken by default.

### Phase 0 — Read-only prep (understand what must not break)
- **Read** `vendor/afterhours/PLUGIN_API.md` and treat it as “what must keep working”.
- Identify which PharmaSea call sites currently use Afterhours APIs:
  - `EntityHelper::*`
  - `EntityQuery`
  - `RefEntity` / `OptEntity`
- Confirm the repository expectation:
  - Existing Afterhours consumers should be able to upgrade and compile with no code changes (default behavior).

Deliverable: short notes (in the PR description or a scratch doc) listing “must-keep” APIs and any direct field access you found in PharmaSea or plugins.

### Phase 1 — Add new handle types (no behavioral changes)
- Add a new public type in Afterhours:
  - `struct EntityHandle { uint32_t slot; uint32_t gen; };`
  - Provide `valid()` and a canonical invalid value.
- Add helper serialization notes (Afterhours doesn’t ship Bitsery, but we want handle to be trivially serializable).
- Add a minimal set of helpers (free functions or methods):
  - `constexpr EntityHandle invalid_handle();`
  - `bool operator==(EntityHandle, EntityHandle);`

Where:
- `vendor/afterhours/src/core/entity.h` is the most discoverable “core types” header.

Deliverable: Afterhours builds; no other code changes required.

### Phase 2 — Add resolver APIs to `EntityHelper` (still no behavior change)
- Add new *static* APIs (additive only):
  - `static EntityHandle handle_of(const Entity& e);` *(can be placeholder initially)*
  - `static Entity* resolve_ptr(EntityHandle h);`
  - `static OptEntity resolve(EntityHandle h);`
- Add documentation comments:
  - resolution is O(1) only once slot store is implemented; initially can fallback.

Where:
- `vendor/afterhours/src/core/entity_helper.h`

Deliverable: Afterhours builds; existing plugins/projects unaffected; new APIs compile.

### Phase 3 — Implement slot bookkeeping internally (compatibility-first)

#### Goal
Get the benefits (stable handles, O(1) resolve, safe invalidation) while keeping:
- `Entities` type unchanged (`vector<shared_ptr<Entity>>`)
- `EntityQuery` unchanged (still iterates dense `Entities`)
- `EntityHelper::get_entities()` unchanged (still returns `const Entities&`)

#### Strategy (recommended): keep `entities_DO_NOT_USE` as the dense live list
Implement “slots/free_list” as an *additional index* alongside the dense vector.

##### Add storage inside `EntityHelper`
Add private-ish members (may still be public in the struct, but treat as internal):
- `struct Slot { EntityType ent; uint32_t gen; uint32_t dense_index; };`
- `std::vector<Slot> slots;`
- `std::vector<uint32_t> free_list;`

##### Implement handle assignment
When an entity becomes “live” (on merge into main list):
- allocate/reuse a slot
- set `slots[slot].ent = shared_ptr`
- set `dense_index` to the index in `entities_DO_NOT_USE`
- return/store handle `{slot, gen}`

##### How to implement `handle_of(const Entity&)` without changing `Entity`
Pick one of these approaches (do the simplest first):
- **A (recommended): store slot index on the entity**
  - Add `uint32_t slot_index` (and maybe `uint32_t slot_gen_snapshot`) to `Entity`.
  - This is *technically* an API change (struct layout), but source-compatible for almost all code.
  - It makes `handle_of` O(1).
- **B: maintain a side map from `Entity* -> slot_index`**
  - This avoids changing `Entity`, but it reintroduces a map-like structure.
  - If you really want to avoid maps, prefer A.

##### Implement `resolve_ptr(handle)`
- Validate bounds: `handle.slot < slots.size()`
- Validate slot live: `slots[slot].ent != nullptr`
- Validate generation: `slots[slot].gen == handle.gen`
- Return `slots[slot].ent.get()` or null if invalid.

Deliverable: Afterhours builds and can resolve handles correctly; existing query/iteration behavior unchanged.

### Phase 4 — Deletion/cleanup integration (the correctness trap)

Afterhours uses the `cleanup` boolean and compacts `entities_DO_NOT_USE` via `erase/remove_if`.
This is where handles can silently break if you don’t update `dense_index` for moved entities.

#### Required behavior
Whenever the dense vector removes or moves elements, you must update slot bookkeeping:
- if you swap-remove manually: update moved entity’s slot `dense_index`
- if you do bulk `remove_if` compaction: you either:
  - re-walk the resulting dense vector and recompute every live slot’s `dense_index`, or
  - stop using bulk `remove_if` and implement explicit swap-removal with slot updates

#### Suggested implementation
- Replace `cleanup()`’s `erase/remove_if` with an explicit loop that:
  - iterates indices,
  - swap-removes dead entities,
  - updates the moved entity’s `dense_index`,
  - invalidates the removed entity’s slot (clear `ent`, increment `gen`, push slot to `free_list`)

Also update:
- `delete_all_entities*()`
- `delete_all_entities(include_permanent)`
- any other code paths that erase from `entities_DO_NOT_USE`

Deliverable: deleting entities does not corrupt handles; stale handles reliably fail generation checks.

### Phase 5 — Keep temp semantics stable (or add opt-in semantics)
Current behavior:
- `createEntityWithOptions` pushes to `temp_entities`
- queries may miss temp entities unless forced merge

Decide and implement one:
- **Keep behavior** (recommended first):
  - temp entities have no valid handle until merged
  - `handle_of(temp_entity)` returns invalid, or asserts in debug
- **Opt-in behavior**:
  - assign handles immediately, but keep the query warnings; temp entities still not returned unless merged

Deliverable: behavior matches existing Afterhours expectations unless a project opts in.

### Phase 6 — Add opt-in query APIs for handles (no break)
Add new API methods to `EntityQuery`:
- `std::vector<EntityHandle> gen_handles() const;`
- `std::optional<EntityHandle> gen_first_handle() const;`

Implementation approach:
- call `gen()` (existing), then map `Entity& -> handle_of(entity)` into handles.

Deliverable: projects can start storing handles from queries without changing existing `gen()` usage.

### Phase 7 — Opt-in breaking changes (strict mode)
Introduce a compile-time option (macro) and only apply breaking aliases under it:
- `AFTER_HOURS_USE_HANDLE_REFS`

Under this macro, you may:
- alias `RefEntity` and/or `OptEntity` to handle-based wrappers
- adjust operators to fail safely (return null/optional) or assert

Deliverable: existing users unaffected; early adopters can flip the macro and migrate with compiler help.

### Phase 8 — Validation & test plan (must do)
Add basic tests (can live in Afterhours examples/tests) that cover:
- create N entities → get handles → resolve works
- mark some for cleanup → cleanup → those handles fail, remaining resolve
- repeated create/delete reuse slots → old handles fail, new handles resolve
- compaction correctness → handle resolution still correct after many deletions
- temp entities behavior remains consistent (based on your decision)

Also add a quick benchmark (optional) showing:
- linear scan `getEntityForID` vs handle resolve under churn.

---

## Questions to answer (to lock down behavior and avoid rework)

These are the key decisions that change implementation details. Answering them up front will save time.

1) **Temp entities**
   - Should temp entities have handles immediately, or only after merge?
   - Should queries ever see temp entities by default?

2) **Lifetime rules for `RefEntity` / `OptEntity`**
   - Do we formally document them as “only valid for immediate use” (current reality)?
   - Do we want new handle-based ref types for “store this for later” use?

3) **Entity struct changes**
   - Is it acceptable to add a `slot_index` field to `Entity` for O(1) `handle_of`?
   - If not, are we okay with a side mapping (which is effectively a map)?

4) **Deletion semantics**
   - Are entities only deleted via `cleanup()` and `delete_all_entities*()`?
   - Are there other code paths that erase from the entity list that must be updated?

5) **Permanent entities**
   - How do permanent entities interact with slots on “delete non-permanent” operations?
   - Do permanent entities keep their handle stable across resets?

6) **Threading model**
   - Is entity creation/deletion single-threaded? (the current global `ENTITY_ID_GEN` suggests “mostly yes”, but confirm)
   - If multi-threaded, we need synchronization around slots/dense/free_list.

7) **Serialization expectations**
   - Do we want Afterhours to provide any built-in serialization hooks for handles, or just keep it as POD and let projects serialize it?
   - Should handle layout be fixed-width for network stability (recommended)?

---

## Answers so far (and implications)

### 1) Temp entities: handle timing + query visibility

You have two knobs here:
- **When does an entity receive a handle?** (on create vs on merge)
- **When do queries see the entity?** (before merge vs after merge)

Afterhours today strongly implies a two-step lifecycle:
- `createEntityWithOptions()` pushes into `temp_entities`
- `merge_entity_arrays()` moves temp → main
- `EntityQuery` warns it will miss temp unless forced merge
- `cleanup()` runs at end-of-frame

#### A) Assign handles only after merge (default; most compatible)
- **Pros**
  - Preserves existing behavior and mental model: “not in the world until merged”.
  - Keeps `EntityQuery` semantics consistent (missing temp is expected).
  - Makes it easy to reason about: `handle.valid() == true` ⇒ entity is in the main dense list.
- **Cons**
  - You can’t create-and-store a handle *immediately* in the same scope unless you force a merge (or delay storing until after merge).

#### B) Assign handles immediately on create (opt-in; more power, more footguns)
- **Pros**
  - You can store relationships (handles) to new entities immediately (useful for spawned pairs, inventories, etc.).
- **Cons**
  - You must define whether `resolve(handle)` works for temp entities (it probably should), even though queries may still not see them by default.
  - This can surprise existing code: “handle resolves, but query doesn’t find it” until merge.
  - If you also make queries see temp by default, you risk behavior changes in existing games.

**Your stated preference**: preserve existing behavior by default, so **A is the default** and **B is opt-in** via a macro / option.

**Decision (confirmed)**:
- **Default**: assign handles only after `merge_entity_arrays()` (Option A).
- **Later (opt-in)**: allow handles to be assigned immediately on create (Option B) behind a macro/flag.

### 2) `RefEntity` / `OptEntity`
Decision: keep them as “real refs” by default (non-owning reference wrappers).
- **Implication**: no existing projects break on update.
- **Implication**: long-lived references should migrate to handles (new types / APIs), but that migration is voluntary.

### 3) Adding slot metadata to `Entity`
Decision: acceptable to add metadata like `slot_index` to `afterhours::Entity`.
- **Implication**: `EntityHelper::handle_of(entity)` can be O(1) without a map.
- **Caller note**: if a project serializes `Entity`, it should treat slot metadata as runtime-only and not persist it as game state (PharmaSea will serialize handles separately).

### 4) Deletion pathways
Decision: only deletion is “mark `cleanup=true` during the frame, then remove at end of frame”.
- **Implication**: handle invalidation can be centralized in `EntityHelper::cleanup()` (plus delete-all helpers).
- **Implementation warning**: because `cleanup()` currently compacts via `erase/remove_if`, it must be rewritten (or followed by a rebuild pass) so slot bookkeeping stays correct.

### 5) Handle stability
Decision: “handles should always remain the same.”

To make this implementable with high churn, interpret this as:
- **A handle remains stable for the lifetime of the entity.**
- After deletion, the handle becomes permanently invalid (generation mismatch), even if the slot is reused.

This is the standard “slot+generation” guarantee and preserves safety under frequent create/delete.

### 6) Threading model (what exists today in Afterhours)
From the code:
- The ECS update loop (`SystemManager::run`) iterates entities in a normal loop, merges temp entities after each system step, and runs `cleanup()` once per frame — this is effectively **single-threaded entity lifetime management**.
- There is at least one plugin (`plugins/pathfinding.h`) that can start a background thread and uses mutexes for its own internal queues.

**Implication**: you can implement slots/free_list/dense assuming entity creation/deletion happens on the main thread (today’s model). If you later want cross-thread entity access, you’ll need explicit rules or synchronization.

**Decision (confirmed)**:
- Treat entity lifetime management (create/merge/cleanup + slot bookkeeping) as **main-thread only**.

### 7) Serialization
Decision:
- For now, **callers serialize handles themselves**.
- Eventually, Afterhours may provide helper serialization utilities.

**Implication**: keep `EntityHandle` as a simple fixed-width POD (two 32-bit integers) and document the recommended wire layout.

### 8) Save/load identity & fixups (project-level behavior, but should be documented)
Based on the latest discussion:
- **EntityID on load**: it’s acceptable to remap IDs on load **as long as** anything storing IDs is updated consistently.
- **Guardrail before changing anything**: audit “random ints” vs “entity IDs” and make sure entity references are strongly typed/tagged (so we don’t miss a field during fixups).
- **ENTITY_ID_GEN**: after load, set the generator to `max_loaded_id + 1` (or equivalent) to avoid collisions.
- **Missing references**: on load, if a reference points to a non-existent entity, **log and clear** (and treat it as data corruption / user-edited save if it happens).
- **Fixup timing**: still undecided; needs a clear rule (see “Open questions” below).

**Important refinement (because PharmaSea is migrating to handles)**:
- After the migration, most gameplay state should store **handles**, not IDs.
- Therefore, the key persistence question becomes: **how do we rehydrate handles on load?**
  - Practical approach: persist relationships as `EntityID` (or a persistent key) in the save, then rebuild handles and fix up after load.
  - Alternative approach: persist raw slot-based handles and reconstruct the exact slot layout on load (tighter coupling; usually not worth it).

---

## Open questions (still need an explicit decision)

### Fixup timing
If a project stores entity references as `EntityID` or `EntityHandle`, you need a consistent rule for when it’s safe to resolve them:
- Option A: run all fixups immediately after entity list deserialization, before any system tick.
- Option B: allow a “partially loaded” state and require systems/components to handle missing references until a later pass.

Pick one and document it as a contract; otherwise load order bugs will be hard to reason about.

---

## Clarification: “ID→handle index structure” (what it means)

When you store relationships as `EntityID` (for network or saves), you usually want a fast way to resolve `EntityID -> Entity*` (or `EntityID -> EntityHandle`) at runtime.

There are three broad choices:

1) **Linear scan** (current Afterhours behavior: loop over `Entities`)
   - Simplest, but O(n) per lookup.

2) **Hash map** (`unordered_map<EntityID, EntityHandle>` or `Entity*`)
   - O(1) average, but involves hashing, allocations, and rehashing under churn.

3) **Vector-backed index** (not a map; fastest, but can grow)
   - Keep a vector `id_to_slot_or_handle` where the array index is the `EntityID`.
   - Example: `id_to_slot[id] = slot_index` (or store a full `EntityHandle`).
   - Lookup becomes O(1) array indexing, no hashing.
   - Tradeoff: memory is proportional to the **maximum EntityID ever assigned**, not current live entities.
     - If your IDs only grow and never reset, this can grow over long sessions.
     - If you reset IDs on new game/load, memory stays bounded.

This “vector-backed index” is what was meant by #6; it’s a common alternative when you want O(1) without a hash map.

**Note for PharmaSea specifically**:
- Once PharmaSea stores long-lived relationships as `EntityHandle`, it should not need frequent `EntityID -> entity` lookups in hot paths.
- That means an ID index structure can remain a legacy/interop tool (networking, debug, or transitional migration) rather than something performance-critical long-term.
