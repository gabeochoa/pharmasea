# Pointer-free serialization plan (PharmaSea)

This plan turns `docs/pointer_free_serialization_options.md` into an execution roadmap for making **network + save serialization contain zero pointers** (no pointer payloads and no pointer-linking context), while keeping runtime performance “pointer-fast” via **handles**.

## Preconditions (already done / verified)

- **Afterhours submodule branch**: `vendor/afterhours` is checked out on `cursor/remove-serializing-pointers-438b`.
- **Recent Afterhours additions (last 25 commits, highlights)**:
  - **Phases 4–6 of “remove serialized pointers”**: component pool/store, entity handle work, and **snapshot persistence + apply** (`src/core/snapshot.h` in Afterhours).
  - **Guardrails**: `src/core/pointer_policy.h` + compile-fail boundary checks.
  - **Perf**: entity query short-circuiting/caching improvements.
  - **Docs/tests/examples**: regression examples for tag filters, component pools, pointer-free snapshots.

## Goal / non-goals

- **Goal**: Bitsery payloads for:
  - `network::ClientPacket::MapInfo` (and any entity serialization it transitively includes)
  - `save_game::SaveGameFile::map_snapshot`
  contain **no raw pointers** and do **not** require `bitsery::ext::PointerLinkingContext`.
- **Goal**: Runtime references stay fast using **O(1) handle resolution** (slot + generation), not O(n) `EntityID` scans.
- **Non-goal**: Rewriting gameplay systems wholesale in one PR. This is an incremental migration with guardrails.

## Current blocking sources of pointer-style serialization in this repo

Even if cross-entity relationships are already mostly `EntityID`-based, the **world snapshot** is still pointer-encoded:

- **Entities are serialized as `std::shared_ptr<Entity>`**:
  - `src/level_info.h`: `s.ext(entity, bitsery::ext::StdSmartPtr{})`
- **Entity components are serialized via `StdSmartPtr`**:
  - `src/entity.h`: `s.ext(ptr, StdSmartPtr{})` for `entity.componentArray[i]`
- **Network/save contexts still include pointer linking**:
  - `src/network/serialization.h`: `TContext` includes `bitsery::ext::PointerLinkingContext`
  - `src/save_game/save_game.cpp`: same tuple context

Until those are removed, Bitsery will keep using pointer-linking machinery (and we’re still “pointer serializing” in practice).

## Strategy choice (“options” → decision)

We will execute **Option 3 (handle system: slot + generation + dense iteration)** from `docs/pointer_free_serialization_options.md`, because it:

- avoids the “ID lookup is O(n)” trap,
- aligns with the direction already implemented in `vendor/afterhours` on `cursor/remove-serializing-pointers-438b`,
- provides the cleanest path to snapshots that are **structural data**, not pointer graphs.

## Phased plan

### Phase 0 — Lock dependencies + establish “no pointers” validation

- **Update rule**: treat `vendor/afterhours` on `cursor/remove-serializing-pointers-438b` as the required baseline for this work.
- **Add validation checks (repo-level)**:
  - A small “serialization audit” check that fails CI if any of these are present in production snapshot code:
    - `bitsery::ext::PointerLinkingContext`
    - `bitsery::ext::StdSmartPtr` used for entity/world snapshot graphs
  - A “binary format sanity” check: serialize+deserialize a world snapshot and assert entity counts + key invariants.

**Exit criteria**: we have an automated way to detect pointer-linking usage in snapshot paths.

### Phase 1 — Adopt a single canonical handle type in PharmaSea

- **Canonical type**: use Afterhours’ `EntityHandle` (slot + generation) as the only long-lived reference type.
- **Wire format**: serialize handle as **two fixed-width integers** (recommended: 2×`uint32_t`) to make network/save stable across platforms.
- **Repo integration tasks**:
  - Ensure PharmaSea has a single “resolve” path: `EntityHandle -> Entity* / OptEntity`.
  - Decide & document null semantics (invalid handle value; clear-on-missing behavior).

**Exit criteria**: handles resolve in O(1) in-game (not via `EntityID` linear scan), and handle serialization is fixed-width and deterministic.

### Phase 2 — Make the runtime entity store handle-native (dense + stable slots)

PharmaSea currently stores entities as `std::vector<std::shared_ptr<Entity>>` and deletes via `erase/remove_if`, which breaks stable indexing.

- **Implement stable slots + dense list** (compatibility-first):
  - Keep a dense list for iteration.
  - Maintain stable slot indices + generation for handles.
  - Ensure deletion is swap-remove and increments generation.
- **Migration approach**:
  - First, implement the storage change behind `EntityHelper` while keeping existing iteration APIs working.
  - Then gradually switch “persistent relationships” to store handles (where any still exist).

**Exit criteria**: entity lifetime (create/cleanup/delete) preserves handle validity guarantees; iteration remains dense and fast.

### Phase 3 — Replace pointer-graph world serialization with snapshot serialization

This is the core “remove pointer-linking” phase.

- **Stop serializing**:
  - `std::shared_ptr<Entity>` entity lists as graphs
  - per-component `StdSmartPtr` pointers
- **Replace with** a pointer-free snapshot format:
  - Use the Afterhours snapshot work (on this branch) as the model:
    - entities serialized as records (id/type/tags + component data)
    - components stored in pools/stores and serialized by index/handle, not pointer identity
- **Refactor targets**:
  - `LevelInfo::serialize` should no longer write a `container(shared_ptr<Entity>)`.
  - `bitsery::serialize(Entity&)` in `src/entity.h` should no longer iterate `componentArray` and serialize smart pointers.

**Exit criteria**: you can serialize a full `Map`/`LevelInfo` snapshot without any use of `StdSmartPtr` or pointer-linking context.

### Phase 4 — Save-game: versioned, pointer-free snapshot + deterministic fixups

- **Introduce `save_version = 2`** (or bump existing version) with an explicit:
  - header (small, previewable)
  - snapshot payload (pointer-free)
- **Fixup policy**:
  - Decide one rule and enforce it: **run all fixups immediately after deserializing the entity/component snapshot and before any system tick**.
  - Missing references: **log + clear** (treat as corrupted/edited save).

**Exit criteria**: save/load round-trips a world without pointer contexts, and fixups are deterministic and centralized.

### Phase 5 — Networking: pointer-free map state packets

- **Replace `ClientPacket::MapInfo` payload** to use the pointer-free snapshot form (full snapshot first; deltas later).
- **Remove pointer-linking context** from network serializer types once no pointer extensions are used.

**Exit criteria**: clients can receive/apply map snapshots without `PointerLinkingContext`, and packet size/perf is acceptable.

### Phase 6 — Remove pointer contexts and enforce “no pointer serialization” permanently

Once Phases 3–5 are complete:

- Remove `bitsery::ext::PointerLinkingContext` from:
  - `src/network/serialization.h`
  - `src/save_game/save_game.cpp`
- Remove unused pointer extensions/includes (`bitsery/ext/pointer.h`, `PointerObserver`, etc.) from snapshot code paths.
- Add/enable guardrails inspired by Afterhours `pointer_policy.h`:
  - “compile-fail” tests or static checks that prevent reintroducing pointer serialization patterns.

**Exit criteria**: repository-wide snapshot/network/save formats are pointer-free by construction, and regressions are blocked by tests.

## Risks / watchouts

- **Compatibility**: switching entity storage semantics can surface latent bugs where code stores `RefEntity` longer than a frame. Make this failure mode explicit (prefer handle storage).
- **Performance**: ensure hot paths do not fall back to `EntityHelper::getEntityForID` linear scans once handles exist.
- **Rollout**: keep save/network versioning explicit; support “old save” migration or clearly gate it by version.

## Concrete “definition of done”

- `network::serialization` and `save_game` serialization contexts no longer include `PointerLinkingContext`.
- No snapshot-path serialization uses `StdSmartPtr` / pointer graph encoding.
- A fresh save loads correctly; a client can join and receive world state correctly.
- Handle resolution is O(1) and stale handles fail safely (generation mismatch).

