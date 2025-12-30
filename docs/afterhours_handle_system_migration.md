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
