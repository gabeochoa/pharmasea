# Merge Held Position Update Systems

**Category:** System Consolidation
**Impact:** ~35 lines saved
**Risk:** Low
**Complexity:** Low

## Current State

2 systems with near-identical logic:
- `UpdateHeldItemPositionSystem` (32 lines)
- `UpdateHeldHandTruckPositionSystem` (27 lines)

Both: check if holding → get held entity → update transform to holder position

## Refactoring

Single templated system:

```cpp
template<typename HolderComponent>
struct UpdateHeldPositionSystem : public System<Transform, HolderComponent> {
    void process(Entity& holder, float) {
        auto& hold = holder.get<HolderComponent>();
        if (hold.empty()) return;

        OptEntity held = hold.item();
        if (!held) { hold.update(nullptr); return; }

        vec3 offset = calculate_hold_offset(holder);
        held->get<Transform>().update(holder.get<Transform>().pos() + offset);
    }
};

// Register both:
systems.add(UpdateHeldPositionSystem<CanHoldItem>{});
systems.add(UpdateHeldPositionSystem<CanHoldFurniture>{});
```

## Impact

Reduces 59 lines to ~25 lines

## Related

This pairs well with unify-canhold-components.md - if CanHoldFurniture and CanHoldHandTruck are unified, this becomes even simpler.

## Files Affected

- `src/system/sixtyfps/update_held_item_system.h`
- `src/system/sixtyfps/update_held_handtruck_system.h`
