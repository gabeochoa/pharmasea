# Reduce Duplicate Entity Filtering Logic

**Category:** Input Handling
**Impact:** ~100 lines saved
**Risk:** Low
**Complexity:** Low

## Current State

`getClosestMatchingFurniture()` called 7+ times with complex inline lambdas:

```cpp
OptEntity match = EQ().getClosestMatchingFurniture(
    player.get<Transform>(), cho.reach(),
    [](const Entity& furniture) {
        if (furniture.is_missing<HasWork>()) return false;
        const HasWork& hasWork = furniture.get<HasWork>();
        return hasWork.has_work();
    });
```

The same filtering logic is duplicated across input handling code, making it hard to maintain and test.

## The Core Problem

Entity filtering happens in two places:
1. **ECS Queries** - `EQ().whereType().whereHas<Component>()` - fluent, composable
2. **Callback Filters** - lambdas passed to `getClosestMatchingFurniture()` - ad-hoc, duplicated

The question: how do we make callback-based filtering as clean as query-based filtering?

## Approach Options

### A. Named Filter Functions (Original Proposal)

```cpp
namespace furniture_filters {
    inline bool has_active_work(const Entity& furniture) {
        if (furniture.is_missing<HasWork>()) return false;
        return furniture.get<HasWork>().has_work();
    }
}

// Usage:
OptEntity match = EQ().getClosestMatchingFurniture(
    transform, reach, furniture_filters::has_active_work);
```

- Simple extraction
- Easy to understand
- Filters are testable
- Doesn't compose well (need new function for each combination)

### B. Composable Filter Predicates

```cpp
// Filter building blocks
auto has_component = []<typename C>() {
    return [](const Entity& e) { return e.has<C>(); };
};

auto component_check = []<typename C>(auto pred) {
    return [pred](const Entity& e) {
        if (e.is_missing<C>()) return false;
        return pred(e.get<C>());
    };
};

// Composition helpers
template<typename... Filters>
auto all_of(Filters... filters) {
    return [=](const Entity& e) { return (filters(e) && ...); };
}

// Usage:
auto filter = all_of(
    has_component<CanHoldItem>(),
    component_check<CanHoldItem>([](auto& c) { return c.empty(); })
);

OptEntity match = EQ().getClosestMatchingFurniture(transform, reach, filter);
```

- More flexible
- Composable
- More complex to understand
- Template-heavy

### C. Extend EntityQuery with Callback Support

```cpp
// Add filter method to EQ that returns callable
auto filter = EQ()
    .whereHas<HasWork>()
    .where([](const Entity& e) { return e.get<HasWork>().has_work(); })
    .as_predicate();  // Returns std::function<bool(const Entity&)>

OptEntity match = EQ().getClosestMatchingFurniture(transform, reach, filter);
```

- Reuses existing query syntax
- Single pattern for all filtering
- Requires afterhours changes

### D. Move Filtering Into the Query System

```cpp
// Instead of getClosestMatchingFurniture with callback...
OptEntity match = EQ()
    .whereFurniture()
    .whereHas<HasWork>()
    .where([](const Entity& e) { return e.get<HasWork>().has_work(); })
    .closestTo(transform, reach);
```

- Most integrated approach
- All filtering through EQ
- Significant API change
- Requires afterhours changes

### E. EntityFilter Component Integration

The codebase already has `EntityFilter` for some filtering. Could extend this:

```cpp
// Define filter specs
const EntityFilter WORKABLE_FURNITURE = EntityFilter()
    .require<HasWork>()
    .where([](const Entity& e) { return e.get<HasWork>().has_work(); });

// Use with existing systems
OptEntity match = EQ().getClosestMatchingFurniture(
    transform, reach, WORKABLE_FURNITURE.matches);
```

- Builds on existing pattern
- Filters are data, not just code
- Could be serialized/configured

### F. Query Method Overloads

```cpp
// Add overloads that accept common filter types
OptEntity getClosestWithWork(const Transform& t, float reach);
OptEntity getClosestEmptyHolder(const Transform& t, float reach);
OptEntity getClosestAccepting(const Transform& t, float reach, EntityType item_type);

// Usage:
OptEntity match = EQ().getClosestWithWork(transform, reach);
```

- Very readable at call site
- Encapsulates common patterns
- Proliferates methods (but maybe that's fine for common cases)

## Questions to Consider

1. **How many unique filter patterns exist?** If only 5-10, named functions (A) or method overloads (F) are sufficient.

2. **Do filters need to compose?** If yes, approach B or D.

3. **Should filters be data or code?** If data (configurable), approach E.

4. **Is this a Pharmasea problem or afterhours problem?** Generic solutions (C, D) benefit both.

5. **What's the testing story?** Named functions (A) are easiest to unit test.

## Recommendation

Start with **Approach A + F combined**:
1. Extract common filters as named functions (`furniture_filters::has_active_work`)
2. Add convenience methods for the most common queries (`getClosestWithWork()`)
3. Keep lambdas for one-off or complex filters

Consider **Approach D** (query integration) as a longer-term afterhours enhancement if filtering patterns keep growing.

## Files Affected

- `src/system/input/input_process_manager.cpp`
- `src/entity_query.h` (if adding convenience methods)
