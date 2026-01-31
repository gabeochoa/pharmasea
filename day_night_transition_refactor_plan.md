# Split Day/Night Transition Systems

## Overview
Refactor the day/night transition logic from `GameLikeUpdateSystem::after()` into separate systems that run when `needs_to_process_change` is true. The systems will be registered between a "calculate" system (that checks the flag) and a "complete" system (that clears the flag).

## Current State
- `RunTimerSystem` sets `needs_to_process_change = true` when calling `start_day()` or `start_night()`
- `GameLikeUpdateSystem::after()` checks the flag and runs all transition logic, then sets it to false
- Transition logic includes day-start and night-start branches with multiple function calls

## Implementation Plan

### 1. Create CalculateDayNightChangeSystem
- **File**: `src/system/calculate_day_night_change_system.h`
- **Purpose**: Detects if a day/night change needs to be processed
- **Behavior**: 
  - Filters by `HasDayNightTimer` component with `EntityType::Sophie` tag
  - Checks `needs_to_process_change` in `should_run()`
  - Stores sophie entity reference for transition systems
  - No-op in `for_each_with()` (just passes through)

### 2. Create Individual Transition Systems
Split the transition logic into separate systems that check `needs_to_process_change` in `should_run()`:

- **ProcessDayStartSystem** - Handles day start logic:
  - `store::generate_store_options()`
  - `store::open_store_doors()`
  - `day_night::on_night_ended()`
  - `day_night::on_day_started()`
  - `delete_floating_items_when_leaving_inround()`
  - `tell_customers_to_leave()`
  - `reset_toilet_when_leaving_inround()`
  - `reset_customer_spawner_when_leaving_inround()`
  - `update_new_max_customers()`
  - `upgrade::on_round_finished()`

- **ProcessNightStartSystem** - Handles night start logic:
  - `store::cleanup_old_store_options()`
  - `day_night::on_day_ended()`
  - `reset_register_queue_when_leaving_inround()`
  - `close_buildings_when_night()`
  - `day_night::on_night_started()`
  - `release_mop_buddy_at_start_of_day()`
  - `delete_trash_when_leaving_planning()`

### 3. Create CompleteDayNightChangeSystem
- **File**: `src/system/complete_day_night_change_system.h`
- **Purpose**: Clears the `needs_to_process_change` flag after all transition systems run
- **Behavior**:
  - Filters by `HasDayNightTimer` component with `EntityType::Sophie` tag
  - Checks `needs_to_process_change` in `should_run()`
  - Sets `needs_to_process_change = false` in `for_each_with()`

### 4. Create register_day_night_systems() function
- **File**: `src/system/afterhours_systems.cpp`
- **Registration order**:
  1. `CalculateDayNightChangeSystem` (checks flag, enables others)
  2. `ProcessDayStartSystem` (runs when flag is true and is_daytime)
  3. `ProcessNightStartSystem` (runs when flag is true and is_nighttime)
  4. `CompleteDayNightChangeSystem` (clears flag)

### 5. Update register_gamelike_systems()
- Call `register_day_night_systems()` at the appropriate point
- Ensure `RunTimerSystem` runs before the day/night systems (it already does)

### 6. Simplify GameLikeUpdateSystem
- Remove the `after()` method entirely
- Remove sophie tracking (no longer needed)
- System becomes a no-op or can be removed if not needed

## Technical Details

### System should_run() Pattern
Each transition system will:
```cpp
virtual bool should_run(const float) override {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
        return timer.needs_to_process_change;
    } catch (...) {
        return false;
    }
}
```

### Finding Sophie
Systems will use `EntityHelper::getNamedEntity(NamedEntity::Sophie)` to find sophie in `should_run()`. This is acceptable since it's a lightweight lookup.

## Files to Create
- `src/system/calculate_day_night_change_system.h`
- `src/system/process_day_start_system.h`
- `src/system/process_night_start_system.h`
- `src/system/complete_day_night_change_system.h`

## Files to Modify
- `src/system/afterhours_systems.cpp` - Add `register_day_night_systems()` and call it
- `src/system/afterhours_systems.h` - Remove `GameLikeUpdateSystem::after()` logic, simplify the system

## Execution Flow

1. **Frame where timer expires:**
   - `RunTimerSystem` runs, calls `start_day()` or `start_night()`, sets `needs_to_process_change = true`
   - Same frame continues...

2. **Next frame (or same frame if systems run in order):**
   - `CalculateDayNightChangeSystem::should_run()` returns true (flag is true)
   - `ProcessDayStartSystem::should_run()` returns true (if is_daytime) OR
   - `ProcessNightStartSystem::should_run()` returns true (if is_nighttime)
   - Transition systems execute their logic
   - `CompleteDayNightChangeSystem::should_run()` returns true
   - `CompleteDayNightChangeSystem::for_each_with()` sets flag to false

3. **Following frames:**
   - All systems' `should_run()` return false (flag is now false)
   - Systems don't execute until next transition

## Notes
- Systems execute in registration order, so the calculate/complete pattern works
- Each system's `should_run()` is evaluated independently, so all will see the flag as true in the same frame
- The complete system clears the flag, preventing re-execution in subsequent frames

