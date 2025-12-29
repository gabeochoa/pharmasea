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
- Entities stored in a pool/arena with **stable slots**.
- Systems iterate a **separate dense list** of live entities (so “loop every frame” stays fast).
- Components store `EntityHandle` (not `Entity*`, not `shared_ptr<Entity>`, not `RefEntity` pointer).
- Serialization writes handles (or their components), never pointers.

### The important detail: stable slots for references + dense list for iteration
The concern “we iterate every frame so we need compaction” is real, but it does **not** conflict with a free-list pool if you avoid iterating the slot array directly.

Use **two structures**:

- **Stable slot storage (never compacts)**
  - `slots[]`: stable indices, reused via `free_list`.
  - Each slot stores:
    - a pointer/owner to the entity (e.g. `std::shared_ptr<afterhours::Entity>` or `std::unique_ptr<...>`)
    - a `generation` counter
    - a `dense_index` back-pointer (where it currently sits in `dense[]`)

- **Dense live list (always compact, used for per-frame loops)**
  - `dense[]`: contiguous list of **live slot indices** (or raw `Entity*` if you want).
  - Deletion is swap-remove (O(1)) and updates exactly **one** moved slot’s back-pointer.

Conceptual sketch:

```cpp
struct EntityHandle {
  uint32_t slot = 0;
  uint32_t gen = 0;
};

struct Slot {
  std::shared_ptr<afterhours::Entity> ent; // or unique_ptr
  uint32_t gen = 0;
  uint32_t dense_index = 0; // valid iff ent != nullptr
};

std::vector<Slot> slots;
std::vector<uint32_t> free_list;  // slot indices available for reuse
std::vector<uint32_t> dense;      // slot indices of live entities (compact)

EntityHandle create_entity() {
  uint32_t s;
  if (!free_list.empty()) { s = free_list.back(); free_list.pop_back(); }
  else { s = (uint32_t)slots.size(); slots.push_back({}); }
  slots[s].ent = std::make_shared<afterhours::Entity>();
  slots[s].dense_index = (uint32_t)dense.size();
  dense.push_back(s);
  return {s, slots[s].gen};
}

void destroy_entity(EntityHandle h) {
  if (h.slot >= slots.size()) return;
  Slot& slot = slots[h.slot];
  if (!slot.ent || slot.gen != h.gen) return; // stale handle

  // remove from dense (swap-remove)
  uint32_t idx = slot.dense_index;
  uint32_t moved_slot = dense.back();
  dense[idx] = moved_slot;
  dense.pop_back();
  slots[moved_slot].dense_index = idx;

  // invalidate slot + recycle
  slot.ent.reset();
  slot.gen++;
  free_list.push_back(h.slot);
}

afterhours::Entity* resolve(EntityHandle h) {
  if (h.slot >= slots.size()) return nullptr;
  Slot& slot = slots[h.slot];
  if (!slot.ent || slot.gen != h.gen) return nullptr;
  return slot.ent.get();
}

// Per-frame iteration (fast + compact):
for (uint32_t s : dense) {
  afterhours::Entity& e = *slots[s].ent;
  // ... systems update ...
}
```

This gives you:
- **Fast iteration** (dense, cache-friendly)
- **Fast lookup** (slot indexing, no hashing, no linear scans)
- **No “constantly updating a map”** — deletion updates only one moved slot’s `dense_index`
- **Safe reference invalidation** via generation counters

### What this looks like with Afterhours in *this repo*
This repo currently centralizes entity lifetime via `EntityHelper`:
- Entities are stored in `using Entities = std::vector<std::shared_ptr<afterhours::Entity>>;`
- Systems commonly iterate `EntityHelper::get_entities()` (directly or via `EQ()`).
- Cross-entity references in gameplay code are often represented as:
  - `EntityID` (already used in many places)
  - `OptEntity` / `RefEntity` (Afterhours wrappers)
  - occasional `std::shared_ptr<Entity>` fields (e.g. `CanHoldItem::held_item`)

An Afterhours-friendly migration usually looks like:

- **Step A: keep system iteration fast**
  - Replace the “owning list” inside `EntityHelper` with:
    - `dense[]` (live entities) for per-frame iteration + queries
    - `slots[]` + `free_list` for stable addressing and fast resolves
  - `EntityHelper::get_entities()` can keep returning a dense container (or provide a `forEachEntity` that iterates `dense[]`).

- **Step B: introduce a serializable handle type**
  - Add a project-level `EntityHandle` (slot+gen) type and serialize it (two `value4b` fields, or one `value8b` packed).
  - Update serialization of reference-like fields to use `EntityHandle` instead of pointer-backed `RefEntity`.

- **Step C: bridge to existing Afterhours types**
  - Where code currently expects `OptEntity` / `RefEntity`, provide helpers:
    - `OptEntity EntityHelper::getEntity(EntityHandle h)` (resolve → `Entity*` → `OptEntity`)
    - `EntityHandle EntityHelper::handle_of(const Entity& e)` (if you store handle/slot index on the entity or in a side table)
  - The key is: **don’t serialize `RefEntity`**; serialize the handle, then resolve to `RefEntity`/`OptEntity` at runtime.

This keeps the “Afterhours ergonomics” (queries returning `RefEntity`, code working with `Entity&`) while ensuring persistence/network formats never contain raw pointers.

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
