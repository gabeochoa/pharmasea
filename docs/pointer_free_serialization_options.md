# Pointer-free Serialization Options

This doc summarizes **at least 3 viable options** to remove **all pointers** from the serialization system, with **pros/cons** and **performance implications**, specifically for this codebase.

---

## Current state (why pointers appear in the serialized format)

### Pointer support is enabled in serialization contexts
Both networking and save-game serialization wire in Bitsery pointer support:

- Network: `src/network/serialization.h` uses:
  - `bitsery::ext::PointerLinkingContext`
  - `bitsery::ext::PolymorphicContext<bitsery::ext::StandardRTTI>`
- Save game: `src/save_game/save_game.cpp` uses the same tuple context.

This makes pointer-based serialization possible across the codebase.

### Raw/observed pointers are serialized today
There are multiple concrete sites where pointers are written:

- **`RefEntity` is serialized as a pointer**
  - `src/entity.h` serializes `RefEntity` via `PointerObserver` with an 8-byte pointer payload.

- **Several components serialize `Entity* parent`**
  - Examples:
    - `src/components/responds_to_day_night.h`
    - `src/components/adds_ingredient.h`
    - `src/components/can_pathfind.h`

- **Some relationships are modeled with smart pointers and serialized as such**
  - Example: `src/components/can_hold_item.h` stores `std::shared_ptr<Entity> held_item` and serializes it via `bitsery::ext::StdSmartPtr`.

### “Pointers are faster than searching” is true *with today’s lookup*
Your current ID→Entity lookup is a linear scan:

- `src/entity_helper.cpp`: `EntityHelper::getEntityForID(EntityID id)` loops over all entities and checks `e->id == id`.

So if you move to ID-based references without improving lookup, you will pay an O(n) cost and it will feel worse than pointer dereference.

---

## Option 1: Serialize **EntityID / Handle IDs**, rebuild pointers after load (most common, minimal change)

### Idea
Store **stable identifiers** in the serialized data (e.g. `EntityID`), not `Entity*` / `RefEntity` pointers or `shared_ptr` graphs. After deserialization, run a **fixup pass** that restores any runtime-only fast links (if desired).

### What it looks like in this codebase
- **Stop serializing `Entity* parent`**
  - For components like `RespondsToDayNight`, `AddsIngredient`, `CanPathfind`, the `parent` pointer is *derivable runtime state* (the entity that owns the component).
  - After load/deserialize, you can set `parent` during component attachment/registration (or a post-load pass).

- **Replace `RefEntity` pointer serialization with ID serialization**
  - Today `RefEntity` is effectively “a reference to an entity” but is serialized as an observed pointer.
  - Instead, serialize a numeric identity such as `EntityID` (and treat missing targets as null/invalid).

- **Replace smart-pointer relationships with IDs where appropriate**
  - Example: `CanHoldItem` currently stores `std::shared_ptr<Entity> held_item` and serializes it.
  - A pointer-free approach stores `EntityID held_item_id` (plus optional cached pointer for runtime speed).

### Performance implications
- **Serialization/Deserialization**
  - Usually **faster and smaller** than pointer-linking:
    - Less bookkeeping (no pointer linking context needed for references).
    - Less risk of “invalid pointer” errors on load.
  - Network validation is easier (IDs are plain integers; you can clamp/validate).

- **Runtime**
  - The performance trade-off depends on how you resolve IDs:
    - With the current `getEntityForID` (linear scan), ID resolution is **O(n)** and can be much slower than pointer dereference.
    - With an index, resolution becomes effectively **O(1)**:
      - `std::unordered_map<EntityID, Entity*>` (fast average, some hashing overhead)
      - A dense table (fastest: array indexing + great cache locality) if IDs are mostly dense/reused in a controlled way.
  - A common pattern is:
    - **Serialize only IDs**
    - Keep an **optional cached pointer** in memory that is rebuilt and invalidated safely.

### Pros
- Minimal conceptual shift: still “entity references”, just expressed as IDs on disk/on wire.
- Removes pointer values from serialized blobs entirely.
- Easier compatibility/versioning: IDs are stable data, pointers are not.

### Cons
- Requires a **post-load fixup** step (or consistent “rebind parent / resolve references” logic).
- Requires **fast ID→Entity lookup** to avoid performance regression.

---

## Option 2: Serialize **table indices** (object tables), not IDs (fastest decode for full snapshots)

### Idea
Serialize the world/snapshot as **arrays** (“tables”) and store references as **indices into those arrays**. An index is still pointer-free, but it resolves to a target in constant time without hashing.

### What it looks like in this codebase
- When serializing a map snapshot (network or save):
  - Emit an `entities[]` array in a deterministic order.
  - Any “entity reference” is stored as `int32 index` into `entities[]` (or `-1` for null).
- On deserialize:
  - Read `entities[]`, construct them, then resolve all indices in a fixup pass.

### Performance implications
- **Serialization size**
  - Very compact (an index can be 32-bit).
- **Deserialize speed**
  - Typically **excellent**:
    - Resolving references is `entities[index]` (no hashing, no scanning).
    - Great cache locality during fixup.
- **Runtime**
  - You can still end up with pointers in memory if you want (just not in serialized form), or you can keep indices/handles as runtime references.

### Pros
- Great fit for **frequent network snapshots** (fast encode/decode).
- Avoids “global ID” concerns inside a single serialized blob.
- Deterministic and simple to validate (bounds checks).

### Cons
- Indices are only meaningful **within that blob**.
  - Harder to do partial updates/deltas unless you design stable ordering rules.
- Requires serializer/deserializer to agree on ordering.
- Can be more invasive than Option 1 if the game code assumes global `EntityID` everywhere.

---

## Option 3: Adopt a true **handle system** (index + generation) and stop using pointers for references (largest safety/perf win, biggest refactor)

### Idea
Replace cross-object references with a compact **handle** like:
- `index` (slot in an entity pool)
- `generation` (increments when a slot is reused)

Then a handle resolves to a live entity in O(1) and stale references are detectable.

### What it looks like in this codebase
- Entities stored in a pool/arena with stable slots.
- Components store `EntityHandle` (not `Entity*`, not `shared_ptr<Entity>`, not `RefEntity` pointer).
- Serialization writes handles (or their components).

### Performance implications
- **Runtime**
  - Close to pointer dereference:
    - 1–2 array reads and a generation check.
  - Often better cache behavior than scattered allocations / shared_ptr graphs.
- **Serialization**
  - Simple and compact (no pointer graph reconstruction).
  - Very robust against invalid references (generation mismatch).

### Pros
- Eliminates “search” while still being pointer-free in serialized data.
- Great safety: stale references become detectable rather than UB.
- Scales well as entity counts grow.

### Cons
- Biggest refactor:
  - Touches many call sites that assume direct pointers/refs.
  - Requires changes to entity lifetime, deletion, reuse policy.

---

## Option 4 (hybrid): IDs for persistence + table indices for snapshots

### Idea
Use different reference encodings depending on the product need:
- **Save game**: store stable `EntityID` references (good for long-lived persistence, easier compatibility).
- **Network snapshots**: store indices into a snapshot-local `entities[]` table (fastest decode/encode).

### Performance implications
- Lets you optimize for:
  - **fastest network replication**, and
  - **most stable save format**
  without forcing one approach everywhere.

### Pros
- Best of both worlds if you frequently ship full map snapshots.
- Keeps save file more stable across versions.

### Cons
- Two formats to maintain (but often worth it).

---

## Recommendation (for this repo)

### Recommended path: Option 1 now, with a fast lookup index; consider Option 4 next

1) **Immediately remove non-essential pointer serialization**
   - **Do not serialize component `parent` pointers** at all (they are runtime-derived; rebuild them after load).
   - Replace `RefEntity` pointer serialization with an **ID-based representation**.

2) **Switch relationship fields to ID-based references**
   - Anywhere you currently serialize `Entity*`, `RefEntity`, `std::shared_ptr<Entity>`, etc. as a relationship, prefer `EntityID` (plus an optional runtime cache).

3) **Fix the performance cliff by replacing linear scans**
   - Replace/augment `EntityHelper::getEntityForID` with an O(1) lookup (hash map or dense table).
   - This preserves the “pointers are faster” benefit in practice, while keeping serialized data pointer-free.

4) **If network snapshots are a performance hotspot**
   - Add Option 2 semantics for the network path (Option 4 hybrid):
     - table indices for snapshots (fastest decode/encode)
     - stable IDs for saves (best long-term compatibility)

This path gives you the biggest near-term win (pointer-free serialized data + simpler correctness) while keeping runtime performance high by addressing the real bottleneck (current O(n) ID lookup).
