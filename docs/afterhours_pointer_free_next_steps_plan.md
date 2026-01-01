# Afterhours + PharmaSea: pointer-free serialization next steps (implementation plan)

This plan consolidates:

- `vendor/afterhours/remove-serializing-pointers-plan.md`
- `docs/afterhours_handle_system_migration.md`
- `docs/pointer_free_serialization_options.md`

Goal: **zero pointers and zero pointer-linking contexts** in save-game + network formats, without performance regressions. The core strategy is: **handles for identity + O(1) resolution + explicit snapshot DTOs**.

---

## What’s already true (current repo state)

- **Afterhours direction is clear**: slot+generation handles + dense iteration + O(1) lookup (see `docs/afterhours_handle_system_migration.md`).
- In PharmaSea (`src/entity.h`), **`RefEntity` and `OptEntity` already serialize as `EntityHandle`** (good).
- Network and save still use Bitsery pointer context:
  - `src/network/serialization.h`: `TContext = tuple<PointerLinkingContext, PolymorphicContext<...>>`
  - `src/save_game/save_game.cpp`: same
- Save currently serializes a full snapshot as **`Map -> LevelInfo -> vector<shared_ptr<Entity>>`**, so pointer-linking stays required until we replace that surface with pointer-free snapshots.

---

## Non-negotiable requirements (definition of done)

- **No persisted pointers**:
  - No `T*`, no `shared_ptr/unique_ptr`, no pointer observer/owner types, and **no `PointerLinkingContext`** in save/network contexts.
- **Runtime safety**:
  - Stale references must be detectable (generation mismatch) and resolve to “missing”, not UB.
- **Performance**:
  - No O(N) `EntityID -> Entity` scans on hot paths.
  - Query APIs must not do redundant work (no “run query twice” patterns).
- **Compatibility (short term)**:
  - Keep existing “query returns `RefEntity`/`OptEntity` for immediate use” ergonomics.
  - Keep temp entity semantics stable by default: **temp isn’t query-visible until merge**.

---

## Phase 0 — Prep and guardrails (1–2 short PRs)

- **Submodule sanity**:
  - Ensure `vendor/afterhours` is present and builds in CI.
  - Record the pinned commit in repo docs if needed.
- **Tripwire tests** (add before refactors):
  - Afterhours: handle lifecycle, slot reuse increments generation, cleanup invalidates handles.
  - PharmaSea: save/load roundtrip for a tiny world, network entity roundtrip, and “no temp entities unless merged” query behavior.
- **Remove debug spam**:
  - `src/network/serialization.cpp` currently prints `typeid(...)` hashes during serialize; gate behind a debug flag or remove so it doesn’t mask real perf/log issues.

Deliverable: green tests + stable baseline.

---

## Phase 1 — Afterhours: handle system + O(1) resolution (compatibility-first)

**Primary reference**: `docs/afterhours_handle_system_migration.md` (Approach 3A).

### 1.1 Implement slot+generation handles in Afterhours

In `vendor/afterhours/src/core/`:

- **Add/standardize handle type**:
  - `struct EntityHandle { uint32_t slot; uint32_t gen; };`
  - Keep it POD and stable-width.
- **Internal slot bookkeeping** (parallel index, while keeping dense entity list intact):
  - Keep `Entities entities_DO_NOT_USE` as the canonical dense list (so `EntityQuery` and `get_entities()` remain stable).
  - Add `slots[]`, `free_list[]`, and per-slot `dense_index` back-pointer.
- **Resolution APIs**:
  - `EntityHelper::handle_for(const Entity&) -> EntityHandle`
  - `EntityHelper::resolve(EntityHandle) -> OptEntity`
  - Optional hot-path: `resolve_ptr(EntityHandle) -> Entity*`

### 1.2 Make `EntityID -> Entity` O(1)

From `vendor/afterhours/remove-serializing-pointers-plan.md`:

- Add an `id_to_slot` (vector-backed) index:
  - `id_to_slot[EntityID] = slot` (or store full handle)
  - Update on merge/create/delete.
- Rewrite `getEntityForID` to use it.

### 1.3 Rewrite cleanup to maintain bookkeeping correctly

- Replace `erase/remove_if`-style compaction with explicit swap-removal so you can:
  - update the moved slot’s `dense_index`
  - clear slot, bump generation, push slot into `free_list`
  - invalidate `id_to_slot` entry

### 1.4 Add opt-in query helpers for long-lived references

Add to `EntityQuery`:

- `gen_handles()` (return `vector<EntityHandle>`)
- `gen_first_handle()` (return `optional<EntityHandle>`)

Deliverable: Afterhours provides fast, safe, pointer-free identity (`EntityHandle`) while preserving current query/container API shape.

---

## Phase 2 — PharmaSea: migrate persisted relationships to handles (component-by-component)

**Primary reference**: `docs/pointer_free_serialization_options.md` (Option 3).

Rule: anything that lives beyond a frame and must be serialized should store **`EntityHandle` (or optional handle)**, not `RefEntity`, not pointers, not smart pointers.

- **Audit and migrate**:
  - Components that store entity relationships should store `EntityHandle`/`optional<EntityHandle>` (or `EntityID` only if it’s strictly “same tick” and never serialized).
  - Replace any serialized `shared_ptr<Entity>` relationship fields with `EntityHandle`.
- **Runtime usage pattern**:
  - `if (auto e = afterhours::EntityHelper::resolve(handle)) { ... }`
  - On missing: clear handle or treat as “target disappeared”.

Deliverable: all *game* persisted state references are handle-based and safe.

---

## Phase 3 — Replace save/network surfaces with pointer-free snapshots (remove `PointerLinkingContext`)

This is the “finish line” for pointer-free serialization.

### 3.1 Introduce explicit snapshot DTOs (pointer-free)

Create a new snapshot layer (location suggestion: `src/serialization/snapshot/`):

- `EntityRecord`:
  - `EntityHandle handle`
  - `EntityID id` (optional but useful for debugging/back-compat)
  - `entity_type`, tags, and any other strictly-value metadata
- Per-component records:
  - `(EntityHandle owner, ComponentDTO value)`
  - DTOs must be **pointer-free** by policy.

### 3.2 Snapshot encode/decode flows

- **Network**:
  - Serialize only snapshot DTOs (no `Entity`, no `shared_ptr` graphs).
  - Apply snapshots by: create/resolve entities from handles, then apply component DTOs.
- **Save-game**:
  - Store snapshot DTOs as the saved world payload.
  - Add versioning so old saves can be migrated or rejected cleanly.

### 3.3 Remove pointer-linking contexts

Once snapshot DTOs are in place:

- Update:
  - `src/network/serialization.h` context to remove `PointerLinkingContext`.
  - `src/save_game/save_game.cpp` context likewise.
- Any remaining polymorphic needs should be handled with **explicit tagged unions** (e.g., variant of DTO types) rather than pointer graphs.

Deliverable: save files + network packets contain no pointers and don’t use Bitsery pointer extensions.

---

## Phase 4 — Performance + correctness hardening

- **Query hot paths**:
  - Ensure `gen_first()` and `has_values()` don’t run queries twice.
  - Ensure early-exit paths don’t allocate vectors.
- **Debug safety**:
  - In debug builds, add lightweight validation/logging for invalid handles.
  - In release builds, keep resolve fast (bounds + gen check, nothing else).
- **Determinism**:
  - Ensure snapshot serialization ordering is stable (sort by handle/ID where needed).

Deliverable: stable performance and reproducible snapshot formats.

---

## Concrete “what to do next” checklist (recommended order)

- [ ] **(Phase 0)** Remove/gate the debug `printf`s in `src/network/serialization.cpp`.
- [ ] **(Phase 1)** Implement `EntityHandle` + slots/free-list/dense-index in `vendor/afterhours` (Approach 3A: keep `Entities` dense).
- [ ] **(Phase 1)** Add `id_to_slot` and make `getEntityForID` O(1).
- [ ] **(Phase 1)** Rewrite cleanup/delete paths to keep slots/dense indices correct and bump generation.
- [ ] **(Phase 1)** Add `EntityQuery::gen_handles()` and `gen_first_handle()`.
- [ ] **(Phase 2)** Audit and migrate any remaining serialized entity relationships in PharmaSea to handles.
- [ ] **(Phase 3)** Add snapshot DTO layer and switch **network first** to snapshot payloads (smaller blast radius than save compatibility).
- [ ] **(Phase 3)** Switch save-game to snapshot payloads; add save versioning + migration hooks.
- [ ] **(Phase 3)** Remove `PointerLinkingContext` from both network and save contexts.

---

## Risks / decisions to lock down early

- **Handle layout**: prefer `uint32_t slot/gen` for stable wire format. If Afterhours currently uses wider types, decide and document the on-wire representation.
- **Temp entity behavior**: default should remain “no handles until merge” *or* “handles on create but still not query-visible”; pick one default and keep the other behind a feature flag.
- **Save/load fixups**: define exactly when it’s valid to resolve references after load (immediate post-load fixup pass is simplest).

