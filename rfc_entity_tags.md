# RFC: Entity Tags/Groups and TaggedQuery (afterhours)

## Summary
Introduce lightweight bitset-based tags on `afterhours::Entity` and a minimal `TaggedQuery` API to:
- Simplify common filters (e.g., store-spawned vs. non-store) and enable bulk operations
- Replace ad-hoc flags like `.include_store_entities()` and marker components used only for filtering
- Keep the feature generic in `vendor/afterhours`, with game-specific tag enums defined in this repo

This is backwards-compatible and requires only a small addition to `Entity` and its serialization.

## Motivation
Today we use:
- A negative filter against a marker component (`IsStoreSpawned`) plus a toggle `.include_store_entities()` to opt-in
- A boolean `entity.cleanup` for immediate delete-on-tick (orthogonal)
- “Permanent” entity creation (lifetime semantics)

These result in scattered and ad-hoc filters. We can centralize filtering with tags:
- `Store` for store-spawned entities
- `Permanent` for lifetime semantics
- `CleanupOnRoundEnd` for bulk deletes

Tags provide:
- Clear, composable semantics for queries (all/any/none)
- Efficient filters using bitset operations
- Bulk operations (e.g., erase all with a tag)

## Goals
- Generic tag mechanism inside `afterhours` (no game-specific names)
- Fast, constant-time checks for tag membership
- Ergonomic AND/OR/NOT tag filters via `TaggedQuery`
- Backwards-compatible serialization

## Non-goals
- Full-fledged type-safe tag registry in the library (games define their own enums)
- Replacing `entity.cleanup` immediate deletion flag (tags represent membership, not one-shot actions)

## Design

### Entity: Tag storage and API (in vendor/afterhours)
- Data
  - `TagBitset tags{}` stored on `afterhours::Entity`
  - Configurable capacity via compile-time constant: `AH_MAX_ENTITY_TAGS` (default 64)
- Types
  - `using TagId = uint8_t;`
  - `using TagBitset = std::bitset<AH_MAX_ENTITY_TAGS>;`
- Methods
  - `void addTag(TagId id);`
  - `void removeTag(TagId id);`
  - `bool hasTag(TagId id) const;`
  - `bool hasAll(const TagBitset& mask) const;`
  - `bool hasAny(const TagBitset& mask) const;`
  - `bool hasNone(const TagBitset& mask) const;`
- Optional typed helpers (header-only), so callers can use `enum class GameTag : uint8_t { ... }`.

Example (library side):
```cpp
#ifndef AH_MAX_ENTITY_TAGS
constexpr size_t AH_MAX_ENTITY_TAGS = 64;
#endif

using TagId = uint8_t;
using TagBitset = std::bitset<AH_MAX_ENTITY_TAGS>;

struct Entity {
  // existing fields...
  TagBitset tags{};

  void addTag(TagId id) { tags.set(id); }
  void removeTag(TagId id) { tags.reset(id); }
  bool hasTag(TagId id) const { return tags.test(id); }

  bool hasAll(const TagBitset& m) const { return (tags & m) == m; }
  bool hasAny(const TagBitset& m) const { return (tags & m).any(); }
  bool hasNone(const TagBitset& m) const { return (tags & m).none(); }
};
```

### TaggedQuery (in vendor/afterhours)
A small tag-focused query that complements component/type predicates used by the game.

- Interface
  - `TaggedQuery(const Entities&)`
  - `TaggedQuery& allOf(const TagBitset&)`
  - `TaggedQuery& anyOf(const TagBitset&)`
  - `TaggedQuery& noneOf(const TagBitset&)`
  - `template<typename Pred> TaggedQuery& where(Pred)` (optional extra predicate)
  - `std::vector<RefEntity> gen() const`
- Matching rule
  - Require all bits in `all`
  - Require at least one bit from `any` if set
  - Require none of the bits in `none`

Sketch:
```cpp
struct TagMask { TagBitset all{}, any{}, none{}; };

class TaggedQuery {
 public:
  explicit TaggedQuery(const Entities& entities) : entities_(entities) {}

  TaggedQuery& allOf(const TagBitset& m) { mask_.all |= m; return *this; }
  TaggedQuery& anyOf(const TagBitset& m) { mask_.any |= m; return *this; }
  TaggedQuery& noneOf(const TagBitset& m) { mask_.none |= m; return *this; }

  template <typename Pred>
  TaggedQuery& where(Pred p) { pred_ = std::move(p); return *this; }

  std::vector<RefEntity> gen() const {
    std::vector<RefEntity> out; out.reserve(entities_.size());
    for (const auto& sp : entities_) {
      if (!sp) continue; Entity& e = *sp;
      if (!e.hasAll(mask_.all)) continue;
      if (mask_.any.any() && !e.hasAny(mask_.any)) continue;
      if (!e.hasNone(mask_.none)) continue;
      if (pred_ && !pred_(e)) continue;
      out.push_back(e);
    }
    return out;
  }

 private:
  const Entities& entities_; TagMask mask_{}; std::function<bool(const Entity&)> pred_{};
};
```

### Serialization
- With `ENABLE_AFTERHOURS_BITSERY_SERIALIZE`, append `tags` to `Entity` serialization using `bitsery`:
  - `s.ext(entity.tags, bitsery::ext::StdBitset{});`
- Place near `componentSet` for locality
- Backwards compatibility: older saves load with `tags = 0` (no tags)

## Integration in this repo (pharmasea)

### Tag definitions
Define a typed enum for game tags in our codebase:
```cpp
enum class GameTag : uint8_t {
  Store = 0,
  Permanent = 1,
  CleanupOnRoundEnd = 2,
  // ... future tags
};

inline afterhours::TagBitset mask(GameTag tag) {
  afterhours::TagBitset m; m.set(static_cast<uint8_t>(tag)); return m;
}

inline void add_tag(afterhours::Entity& e, GameTag tag) {
  e.addTag(static_cast<uint8_t>(tag));
}
```

### Assign tags
- Store-spawned entities: set `GameTag::Store`
- Permanent entities: set `GameTag::Permanent` in `createPermanentEntity`
- Round-lifetime entities: set `GameTag::CleanupOnRoundEnd`

### Replace ad-hoc filters
- Current default excludes store entities via `Not(WhereHasComponent<IsStoreSpawned>)` unless `.include_store_entities()` is set
- New approach:
  - Default exclusions use tag masks: e.g., `.noneOf(mask(GameTag::Store))`
  - Include store when needed by omitting that exclusion or specifying `.anyOf(mask(GameTag::Store))`
- Replace sentinel component checks (`IsStoreSpawned`) with tag checks where appropriate
- Keep `entity.cleanup` for immediate deletes; tags handle membership and bulk ops

### Bridging helpers (optional)
Add minimal sugar to existing `EntityQuery` to keep call sites ergonomic:
```cpp
struct EntityQuery {
  // ... existing
  EntityQuery& whereHasTag(GameTag tag);
  EntityQuery& whereAnyTag(const afterhours::TagBitset& mask);
  EntityQuery& whereNoTag(GameTag tag);
};
```
These would internally delegate to `afterhours::TaggedQuery` or reuse `Entity` tag predicates during partitioning.

## Examples

### Exclude store by default
```cpp
auto ents = afterhours::TaggedQuery(EntityHelper::get_entities())
  .noneOf(mask(GameTag::Store))
  .gen();
```

### Include store for specific queries (e.g., floor markers)
```cpp
auto ents = afterhours::TaggedQuery(EntityHelper::get_entities())
  .anyOf(mask(GameTag::Store))
  .where([](const Entity& e){ return e.has<IsFloorMarker>(); })
  .gen();
```

### Bulk cleanup at round end
```cpp
for (auto& e : afterhours::TaggedQuery(EntityHelper::get_entities())
                  .allOf(mask(GameTag::CleanupOnRoundEnd))
                  .gen()) {
  e.get().cleanup = true; // or remove immediately in a bulk op
}
```

### Protect permanent entities
```cpp
for (auto& e : afterhours::TaggedQuery(EntityHelper::get_entities())
                  .noneOf(mask(GameTag::Permanent))
                  .gen()) {
  // eligible for full wipe
}
```

## Performance
- Tag checks are constant-time bitset ops
- Query remains O(N) over live entities, like current `EntityQuery`
- Memory: one bitset per entity (64 bits by default)

## Migration Plan
1. Add tags and `TaggedQuery` to `vendor/afterhours` (generic)
2. In this repo:
   - Introduce `GameTag` enum and helpers
   - Tag entities at creation/modification sites
   - Replace `.include_store_entities()` occurrences with tag-based filters
   - Replace `IsStoreSpawned` usage in filters with tag checks where suitable
3. Keep `entity.cleanup` semantics unchanged
4. Optionally remove marker-only components over time

## Alternatives Considered
- Only using marker components: works but spreads logic and requires component presence checks everywhere
- String-based tags: flexible but slower and more error-prone
- Storing tags outside `Entity`: complicates serialization and ownership

## Open Questions
- Tag capacity: is 64 sufficient, or should we raise to 128? (compile-time switch)
- Built-in bulk ops in `afterhours` (`erase_tagged`, `count_tagged`) vs. leaving to consumers
- Should `afterhours` expose typed tag helpers or keep only index-based API?

## Backwards Compatibility
- Saves without tags load with zero-initialized tag bitset
- Network: no change unless `Entity` is serialized over the wire; if so, tags are included as an additional bitset field and default to zero

## Rollout
- Submit PR to `afterhours` with `Entity` tag field, API, `TaggedQuery`, and serialization guard
- Update this repo to use tags gradually, starting with `.include_store_entities()` replacements

## Appendix: Drop-in serialization change (bitsery)
Where `Entity` is serialized (guarded by `ENABLE_AFTERHOURS_BITSERY_SERIALIZE`):
```cpp
s.ext(entity.componentSet, bitsery::ext::StdBitset{});
s.ext(entity.tags, bitsery::ext::StdBitset{}); // NEW
s.value1b(entity.cleanup);
// ... existing componentArray map
```