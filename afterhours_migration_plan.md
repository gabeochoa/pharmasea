# Afterhours Migration Plan

## What's Already Done ✅

These phases are **complete** and don't need any more work:

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Serialization Compatibility | ✅ Done |
| 1.1 | EntityQuery → afterhours::EntityQuery | ✅ Done |
| 1.2 | SystemManager uses afterhours systems | ✅ Done |
| 2.8 | Font Helper Utilities | ✅ Done |
| 2.9 | Color Utilities | ✅ Done |

---

## What Still Needs To Be Done

### Task 1: Move Functions Out of `system_manager.cpp` ✅ DONE

**Status**: Complete. Reduced `system_manager.cpp` from 2489 to 2148 lines (-341 lines).

**What was done**:
- Moved night-start functions to `process_night_start_system.h`:
  - `close_buildings_when_night`, `release_mop_buddy_at_start_of_day`, `delete_trash_when_leaving_planning`, `reset_register_queue_when_leaving_inround`
  - `day_night::on_day_ended`, `day_night::on_night_started`
  - `store::cleanup_old_store_options`
  - `move_player_out_of_building_SERVER_ONLY` (helper)
- Moved day-start functions to `process_day_start_system.h`:
  - `delete_floating_items_when_leaving_inround`, `delete_held_items_when_leaving_inround`, `reset_max_gen_when_after_deletion`
  - `tell_customers_to_leave`, `reset_toilet_when_leaving_inround`, `reset_customer_spawner_when_leaving_inround`, `update_new_max_customers`
  - `day_night::on_night_ended`, `day_night::on_day_started`
  - `store::generate_store_options`, `store::open_store_doors`
  - `upgrade::on_round_finished`
- Updated `afterhours_systems.h` to remove moved declarations

**How to do it**:

1. Open `src/system/afterhours_systems.h` and find a function like:
   ```cpp
   void delete_trash_when_leaving_planning(Entity& entity);
   ```

2. Find its implementation in `src/system/system_manager.cpp`:
   ```cpp
   void delete_trash_when_leaving_planning(Entity& entity) {
       // ... implementation here ...
   }
   ```

3. Find the system file that uses it. Search for the function name:
   ```bash
   grep -r "delete_trash_when_leaving_planning" src/system/*_system.h
   ```
   You'll find it's used in `process_night_start_system.h`.

4. Move the function into that `*_system.h` file (above the system struct that uses it).

5. Remove the function from `system_manager.cpp` and remove its declaration from `afterhours_systems.h`.

6. Build and test to make sure it still works.

**Functions to move** (in order of priority):

| Function | Move to file |
|----------|--------------|
| `delete_trash_when_leaving_planning` | `process_night_start_system.h` |
| `close_buildings_when_night` | `process_night_start_system.h` |
| `release_mop_buddy_at_start_of_day` | `process_night_start_system.h` |
| `reset_register_queue_when_leaving_inround` | `process_night_start_system.h` |
| `day_night::on_day_ended` | `process_night_start_system.h` |
| `day_night::on_night_started` | `process_night_start_system.h` |
| `delete_floating_items_when_leaving_inround` | `process_day_start_system.h` |
| `delete_held_items_when_leaving_inround` | `process_day_start_system.h` |
| `reset_max_gen_when_after_deletion` | `process_day_start_system.h` |
| `tell_customers_to_leave` | `process_day_start_system.h` |
| `reset_toilet_when_leaving_inround` | `process_day_start_system.h` |
| `reset_customer_spawner_when_leaving_inround` | `process_day_start_system.h` |
| `update_new_max_customers` | `process_day_start_system.h` |
| `day_night::on_night_ended` | `process_day_start_system.h` |
| `day_night::on_day_started` | `process_day_start_system.h` |
| `upgrade::on_round_finished` | `process_day_start_system.h` |
| `store::cleanup_old_store_options` | `process_night_start_system.h` |
| `store::generate_store_options` | `process_day_start_system.h` |
| `store::open_store_doors` | `process_day_start_system.h` |
| `store::move_purchased_furniture` | `process_day_start_system.h` or new file |

---

### Task 2: Replace `for_each_old` with Afterhours Patterns (Medium)

**Goal**: Replace uses of `SystemManager::get().for_each_old(...)` with proper afterhours system iteration.

**Why**: `for_each_old` is a legacy pattern. Afterhours systems should iterate entities using their `for_each_with()` method.

**Where to find them**: Search for `for_each_old` in `src/system/*_system.h`:
```bash
grep -r "for_each_old" src/system/*_system.h
```

**Example - Before**:
```cpp
struct OnDayEndedSystem : public afterhours::System<> {
    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { day_night::on_day_ended(entity); });
    }
};
```

**Example - After**:
```cpp
struct OnDayEndedSystem : public afterhours::System<Entity> {
    virtual void for_each_with(Entity& entity, float) override {
        day_night::on_day_ended(entity);
    }
};
```

**Files to update**:
- `src/system/process_night_start_system.h` (has ~6 uses)
- `src/system/process_day_start_system.h` (has ~1 use)

---

### Task 3: Migrate EntityHelper (Medium)

**Goal**: Make `EntityHelper` use afterhours entity creation instead of raw `new Entity()`.

**File**: `src/entity_helper.cpp`

**What to change**:

1. Find line ~91 where it says:
   ```cpp
   Entity* e = new Entity();
   ```

2. Change it to:
   ```cpp
   auto e = std::make_shared<Entity>();
   ```

3. Update the rest of the function to work with `shared_ptr` instead of raw pointer.

4. Test that entity creation still works.

---

### Task 4: Migrate Bitset Utils (Easy)

**Goal**: Use `afterhours::bitset_utils` instead of custom `src/engine/bitset_utils.h`.

**How to do it**:

1. Find all files using bitset_utils:
   ```bash
   grep -r "bitset_utils::" src/
   ```

2. Change the include from:
   ```cpp
   #include "engine/bitset_utils.h"
   ```
   to:
   ```cpp
   #include "ah.h"  // includes afterhours bitset_utils
   ```

3. Change namespace from `bitset_utils::` to `afterhours::bitset_utils::`.

4. Once all uses are migrated, delete `src/engine/bitset_utils.h`.

---

### Task 5: Migrate Library Pattern (Medium)

**Goal**: Use `afterhours::Library<T>` instead of custom `src/engine/library.h`.

**Files that use Library**:
- `src/engine/texture_library.h`
- `src/engine/font_library.h`
- `src/engine/anim_library.h`
- `src/engine/model_library.h`
- `src/engine/sound_library.h`
- `src/engine/music_library.h`
- `src/engine/shader_library.h`

**How to do it**:

1. Look at afterhours `Library<T>` in `vendor/afterhours/src/library.h`.

2. For each library file, change the base class from pharmasea's Library to afterhours::Library.

3. The main difference is error handling - afterhours uses `tl::expected` for better error messages.

4. Test that loading resources still works.

5. Once all libraries are migrated, delete `src/engine/library.h`.

---

### Task 6: Standardize Includes (Easy)

**Goal**: All files should include `"ah.h"` instead of `"afterhours/ah.h"` or direct afterhours paths.

**How to do it**:

1. Find files with direct afterhours includes:
   ```bash
   grep -r "afterhours/" src/ --include="*.h" --include="*.cpp"
   ```

2. Change each one from:
   ```cpp
   #include "afterhours/ah.h"
   // or
   #include "../../vendor/afterhours/src/library.h"
   ```
   to:
   ```cpp
   #include "ah.h"
   ```

3. The wrapper `src/ah.h` already sets all the right flags.

---

### Task 7: Process Trash System TODO (Easy)

**Goal**: Address the TODO in `rendering_system.cpp` about trash.

**File**: `src/system/rendering_system.cpp` (see line with TODO about trash)

**What to do**:
1. Read the TODO comment
2. Decide if action is needed
3. Either fix it or remove the TODO if it's no longer relevant

---

## Optional Future Work (Low Priority)

These are nice-to-have but not urgent:

| Task | Description |
|------|-------------|
| Window Manager Plugin | Replace `rez::ResolutionExplorer` with afterhours plugin |
| Input System Plugin | Evaluate if afterhours input system fits the game |
| UI Plugin | Evaluate afterhours UI vs current UI system |
| Autolayout Plugin | Use afterhours autolayout for UI layout |
| Drawing Helpers | Use afterhours drawing helpers for rounded rectangles |

---

## How to Test Your Changes

After each change:

1. **Build the project**:
   ```bash
   make
   ```

2. **Run the game** and check that:
   - Day/night transitions work
   - Store opens and closes correctly
   - Entities spawn and despawn properly
   - No crashes or errors in the logs

3. **If something breaks**, undo your change and try again with smaller steps.

---

## File Reference

| File | Purpose |
|------|---------|
| `src/system/afterhours_systems.h` | Function declarations called by systems |
| `src/system/afterhours_systems.cpp` | System registration |
| `src/system/system_manager.cpp` | Function implementations (shrink this!) |
| `src/system/*_system.h` | Individual system files |
| `src/ah.h` | Afterhours wrapper with config flags |

---

## Summary

**Remaining work in priority order**:

1. ✅ **Task 1**: Move functions out of `system_manager.cpp` - Makes code easier to navigate
2. ⬜ **Task 2**: Replace `for_each_old` with proper patterns - Cleaner system code
3. ⬜ **Task 3**: Migrate EntityHelper - Use smart pointers
4. ⬜ **Task 4**: Migrate Bitset Utils - Simple find/replace
5. ⬜ **Task 5**: Migrate Library pattern - Better error handling
6. ⬜ **Task 6**: Standardize includes - Consistency
7. ⬜ **Task 7**: Process trash TODO - Cleanup

**Time estimate**: Tasks 1, 4, 6, 7 are ~30 mins each. Tasks 2, 3, 5 are ~1-2 hours each.

---

## Checklist for Each Task

When you complete a task, change `⬜` to `✅` in the summary above.
