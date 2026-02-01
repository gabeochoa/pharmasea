# Unify CanHoldFurniture and CanHoldHandTruck

**Category:** Component Consolidation
**Impact:** ~50 lines saved
**Risk:** Low
**Complexity:** Low

## Current State

Two nearly identical components (~50 lines each):
- `CanHoldFurniture` - `EntityRef held_furniture`, `vec3 pos`
- `CanHoldHandTruck` - `EntityRef held_hand_truck`, `vec3 pos`

Both have identical methods: `empty()`, `is_holding()`, `update()`, `picked_up_at()`, `*_id()`

## Refactoring

Single generic component:

```cpp
struct CanHoldEntity : public BaseComponent {
    EntityRef held{};
    vec3 pickup_pos{};

    bool empty() const { return !held.is_valid(); }
    bool is_holding() const { return held.is_valid(); }
    OptEntity item() const { return held.entity(); }
    void update(Entity* e, EntityID holder_id) { /* ... */ }
    void update(std::nullptr_t) { held = EntityRef{}; }
    vec3 picked_up_at() const { return pickup_pos; }

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self), self.held, self.pickup_pos);
    }
};

// If type distinction needed for queries:
using CanHoldFurniture = CanHoldEntity;
using CanHoldHandTruck = CanHoldEntity;
// Or use separate instances with a type field
```

## Impact

- Removes 1 component file
- Can also merge `UpdateHeldItemPositionSystem` and `UpdateHeldHandTruckPositionSystem` (see merge-held-position-systems.md)

## Files Affected

- `src/components/can_hold_furniture.h`
- `src/components/can_hold_handtruck.h`
