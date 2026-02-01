# Extract Input System should_run() Logic

**Category:** System Consolidation
**Impact:** ~50 lines saved
**Risk:** Very Low
**Complexity:** Very Low

## Current State

5 input systems have nearly identical `should_run()` implementations:
- `CartManagementSystem`
- `PopOutWhenCollidingSystem`
- `UpdateHeldFurniturePositionSystem`
- `ResetEmptyWorkFurnitureSystem`
- `PassTimeForActiveFishingGamesSystem`

Each repeats:

```cpp
virtual bool should_run(const float) override {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        if (hastimer.needs_to_process_change) return false;
        return hastimer.is_bar_open();  // or is_bar_closed() for inround systems
    } catch (...) {
        return false;
    }
}
```

## Refactoring

Extract to utility function:

```cpp
// In system_utilities.h
namespace system_utils {

inline bool should_run_planning_system() {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        if (hastimer.needs_to_process_change) return false;
        return hastimer.is_bar_closed();  // planning = bar closed
    } catch (...) {
        return false;
    }
}

inline bool should_run_inround_system() {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        if (hastimer.needs_to_process_change) return false;
        return hastimer.is_bar_open();  // inround = bar open
    } catch (...) {
        return false;
    }
}

} // namespace system_utils

// Usage in systems:
virtual bool should_run(const float) override {
    return system_utils::should_run_planning_system();
}
```

## Impact

Reduces ~50 lines across 5 systems, centralizes game state logic

## Files Affected

- `src/system/input/planning/cart_management_system.h`
- `src/system/input/planning/pop_out_when_colliding_system.h`
- `src/system/input/planning/update_held_furniture_position_system.h`
- `src/system/input/inround/reset_empty_work_furniture_system.h`
- `src/system/input/inround/pass_time_for_active_fishing_games_system.h`
