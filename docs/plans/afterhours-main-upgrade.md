# Upgrade vendor/afterhours to main

**Date:** 2026-03-06
**Current pin:** `9a11df7` (detached HEAD)
**Target:** `origin/main` (`f37e6e6`, 175 commits ahead)
**Relationship:** Clean fast-forward (no local divergence)

---

## NEED — Compile blockers

### 1. `systems.render()` signature change

`render()` now takes `Entities&` instead of `const Entities&`.

**Where:** `src/system/core/system_manager.cpp:366-372` — `render_entities()` passes `const Entities&`.

**Fix:** Add `const_cast` (same pattern already used on line 363 for `input_systems.tick()`), or change `render_entities` to take `Entities&`.

**Pros:**
- Tiny change, one line — already have the `const_cast` pattern in the same file
- Aligns with afterhours' direction that render systems can mutate (e.g. caching, lazy init)
- Removes the need for `mutable` on `SystemManager systems` member

**Cons:**
- None meaningful — this is mechanical

### 2. Render systems: non-const `for_each_with` is now primary

Afterhours now calls the mutable overload first. `RenderEntitySystem` only implements `for_each_with(const Entity&, const Transform&, float) const`.

**Where:** `src/system/rendering/systems/render_entity_system.h`

**Fix (pick one):**
- Add a mutable overload that delegates to the const one
- Define `AFTERHOURS_SINGLE_RENDER_PASS` in build flags to keep current behavior

**Pros:**
- Only 3 render systems to update (`OnFrameStartSystem`, `RenderWalkableSpotsSystem`, `RenderEntitySystem`)
- `RenderEntitySystem` already has `mutable bool debug_mode_on` — switching to non-const lets that become a normal member, which is cleaner
- Allows render systems to cache data, lazy-load resources, etc. without `mutable` hacks

**Cons:**
- Without `AFTERHOURS_SINGLE_RENDER_PASS`, afterhours calls render systems twice per frame (mutable then const) — wastes cycles if you don't need it
- Recommendation: define `AFTERHOURS_SINGLE_RENDER_PASS` to keep single-pass behavior, but still switch render systems to mutable overloads for cleanliness

### 3. `font_helper.h` refactored to backend dispatch

Body moved to `backends/raylib/font_helper.h`. Should be transparent since pharmasea defines `AFTER_HOURS_USE_RAYLIB`, but verify it compiles.

**Where:** `src/preload.cpp` includes `afterhours/src/font_helper.h`

**Pros:**
- Zero code changes expected — the dispatch header includes the raylib backend automatically
- Cleaner separation if afterhours ever needs backend-specific font behavior

**Cons:**
- If the `afterhours` namespace wrapper was removed from the dispatch header but kept in the backend header, symbols might not resolve — needs a compile check
- Minor: adds one more level of include indirection

---

## SHOULD — Bug fixes you likely have today

### 4. `SINGLETON_FWD`: `static` -> `inline` (ODR fix)

Each translation unit got its own `shared_ptr`, so singletons weren't truly shared across TUs. Pharmasea uses `SINGLETON_FWD` in 15 places (`SystemManager`, `KeyMap`, `Preload`, `Profiler`, `MenuState`, `GameState`, `RecipeLibrary`, `ModelLibrary`, `ModelInfoLibrary`, `TextureLibrary`, `TextureAtlasLibrary`, `ShaderLibrary`, `FontLibrary`, `AnimLibrary`, `ConfigValueLibrary`).

**Pros:**
- Fixes a real correctness bug — singletons were fragmented across TUs
- Zero code changes needed in pharmasea
- May fix subtle bugs that were previously unexplainable (state not persisting, double initialization)

**Cons:**
- Could surface latent bugs — code that accidentally relied on per-TU singletons (e.g. separate initialization in different compilation units) might behave differently
- Very unlikely to cause problems in practice

### 5. `ENTITY_ID_GEN`: `static` -> `inline` (ODR fix)

Entity IDs could collide across translation units.

**Pros:**
- Fixes potential entity ID collisions — critical for ECS correctness
- Zero code changes in pharmasea
- Networking relies on unique entity IDs — this could fix desyncs

**Cons:**
- Same as #4: could surface latent bugs, but extremely unlikely to cause regressions

### 6. `consume_press` delivered key to wrong action

When multiple actions share a key, `consume_press` delivered to the wrong one. Pharmasea uses `consume_pressed` in 12 call sites across pause menu, recipe book, seed manager, debug settings, and focus navigation.

**Pros:**
- Fixes real input bugs — e.g. Pause and ShowRecipeBook both consume presses, could interfere
- Zero code changes in pharmasea (pharmasea wraps this in `input_helper::consume_pressed`)

**Cons:**
- Could change behavior players have adapted to — if the "wrong" action was consistently consumed, fixing it might feel different
- Low risk: the fix makes behavior correct, not different

### 7. macOS `is_mouse_button_pressed` fix

Raylib's `IsMouseButtonPressed` was broken due to `glfwSwapBuffers` pumping the Cocoa event queue. Now uses manual edge detection.

**Pros:**
- Fixes mouse click reliability on macOS
- Zero code changes in pharmasea — pharmasea doesn't call `is_mouse_button_pressed` directly (afterhours handles it internally)

**Cons:**
- None — pure bugfix, only affects macOS, no API change

### 8. ECS performance optimizations

- O(1) `markIDForCleanup` (was linear scan via `id_to_slot`)
- `unordered_map` for singleton lookup (was `std::map`)
- `make_shared` instead of raw `new` for entities
- Single-pass query filtering (was multi-pass with `std::partition`)
- Fixed `reserve_temp_space` sizeof bug
- Entity iteration avoids `shared_ptr` refcount bumps (`const auto&` instead of copy)

**Pros:**
- Free performance across the board — no code changes needed
- `markIDForCleanup` O(1) is significant: pharmasea deletes entities every frame (customers leaving, trash, floating items)
- Query optimization matters: pharmasea has 25+ `add_mod()` calls across custom query filters
- Singleton lookup speedup helps — pharmasea accesses singletons frequently in hot paths

**Cons:**
- `unordered_map` has worse worst-case than `std::map` (hash collisions) — irrelevant at pharmasea's scale
- No real downsides — all changes are internal to afterhours

---

## COULD — New features to consider

### 9. `should_iterate()` override on systems

Systems can override `should_iterate()` to return `false`, skipping entity iteration while still getting `once()`/`after()` calls.

**Pros:**
- 6 systems currently use `once()` for per-frame work without needing per-entity iteration (`OnFrameStartSystem`, `RenderWalkableSpotsSystem`, `GenerateStoreOptionsSystem`, `CleanUpOldStoreOptionsSystem`, `ResetCustomersThatNeedResettingSystem`, `CountInBuildingTriggerAreaEntrantsSystem`)
- Cleaner intent: explicitly says "this system doesn't iterate entities" rather than having an empty `for_each_with`
- Small perf win: avoids iterating all entities just to do nothing per-entity

**Cons:**
- Requires auditing those 6 systems and changing them — not free
- Low priority: the current pattern (empty `for_each_with` or no override) works fine
- Risk of accidentally skipping iteration on a system that actually needs it

### 10. `on_iteration_begin()` / `on_iteration_end()` hooks

Called once before/after the entity iteration loop within a system.

**Pros:**
- Some systems do setup in `once()` that's conceptually "before iteration" — this makes intent clearer
- Could replace the pattern of using `once()` for pre-iteration setup and `after()` for post

**Cons:**
- Pharmasea doesn't have systems that obviously need this — current `once()`/`after()` pattern works
- Adds API surface to learn and maintain
- Not worth adopting unless a new system specifically needs it

### 11. KeyChord input system

`KeyChord` struct with modifier support (`MOD_SHIFT`, `MOD_CTRL`, `MOD_ALT`, `MOD_SUPER`). Enables combos like `Shift+Left` as first-class input mappings.

**Pros:**
- Could enable debug shortcuts (Ctrl+D for debug mode, Ctrl+R for reload) without conflicting with normal keys
- `AnyInput` variant now uses `KeyChord` instead of `KeyCode` as first alternative — pharmasea defines its own `AnyInput` so no breakage, but could adopt for richer bindings
- `pressed_exact()` method prevents bare-key actions from firing when a chord already claimed that key

**Cons:**
- Pharmasea defines its own `AnyInput` type (`std::variant<int, GamepadAxisWithDir, GamepadButton>`) and its own input checking — adopting KeyChord means either migrating to afterhours' input checking or bridging the two
- Current game doesn't need modifier combos — it's a couch co-op game, players use simple keys and gamepads
- Significant refactor of `keymap.h`, `input_helper.cpp`, and all mapping setup code to adopt
- Not worth the cost unless pharmasea adds complex keyboard shortcuts

### 12. `add_filter(lambda)` on EntityQuery

New `add_filter(std::function<bool(const Entity&)>)` alongside existing `add_mod(Modification*)`.

**Pros:**
- Could simplify some of the 25 `add_mod(new ...)` call sites in `entity_query.cpp` — lambdas are more readable than allocating `Modification` subclasses
- No migration required — `add_mod()` still works unchanged
- Could incrementally adopt: use `add_filter()` for new queries, keep `add_mod()` for existing ones

**Cons:**
- `std::function` has overhead vs the current `Modification*` approach (heap allocation, type erasure)
- Existing `add_mod()` pattern is well-established in the codebase — mixing two styles adds cognitive load
- Not worth bulk-migrating existing queries — only adopt for new code

### 13. Automatic gamepad mapping loading

Afterhours' `add_singleton_components()` now calls `try_load_gamepad_mappings()` which auto-loads `resources/gamecontrollerdb.txt`.

**Pros:**
- Pharmasea already has `gamecontrollerdb.txt` in `resources/` and manually loads it via `KeyMap::load_controller_db()` → `Files::game_controller_db()` → `ext::set_gamepad_mappings()`
- Could remove pharmasea's manual loading code (~15 lines in `keymap.cpp`)

**Cons:**
- Afterhours looks for the file at a hardcoded relative path — pharmasea uses `Files::resource_folder()` which resolves platform-specific paths
- If afterhours loads it before pharmasea's `Files` singleton is initialized, it might look in the wrong directory
- The manual loading works reliably — replacing it with auto-loading risks subtle path resolution bugs
- Not worth the risk for 15 lines of code savings

### 14. 3D drawing helpers (`drawing_helpers_3d.h`)

`get_world_to_screen()`, `begin_3d()`, `end_3d()`, `draw_cube()`, `draw_sphere()`, `draw_line_3d()`, etc.

**Pros:**
- Maps directly to the BillboardText/WorldLabel todo item — `get_world_to_screen()` is exactly what's needed to project 3D positions to screen coordinates for text rendering
- Wraps raylib calls in afterhours namespace, consistent with the rest of the codebase
- If pharmasea ever moves to a different backend, these abstractions help

**Cons:**
- Pharmasea already calls raylib 3D functions directly throughout `rendering_system.cpp` (~1500 lines) — wrapping in afterhours adds indirection for no immediate benefit
- Only `get_world_to_screen()` is actually needed for the billboard todo; the rest (`draw_cube`, etc.) duplicate what pharmasea already does
- Can use raylib's `GetWorldToScreen()` directly when implementing BillboardText — no need for the wrapper

### 15. Separate UI entity collection

UI entities live in a dedicated `EntityCollection` so game ECS systems don't iterate over UI elements. Opt-in via `AFTER_HOURS_UI_SINGLE_COLLECTION` define.

**Pros:**
- Pharmasea's game systems iterate all entities every frame — separating UI entities reduces iteration count
- Prevents accidental interactions between game systems and UI entities
- Clean architectural boundary

**Cons:**
- Pharmasea doesn't currently use afterhours UI entities — its UI is rendered via its own `ui::render_normal()` function, not through the ECS
- Adopting this requires moving pharmasea's UI to afterhours' UI entity system first, which is a massive refactor
- The `AFTER_HOURS_UI_SINGLE_COLLECTION` flag exists for migration, but there's nothing to migrate yet
- Not relevant until pharmasea adopts afterhours UI components

---

## NOT relevant (skip)

~100 of the 175 commits cover Sokol/Metal backend, UI widget framework (dropdowns, toggles, tabs, text_input, tree_view, etc.), E2E testing infrastructure, and design docs. Pharmasea doesn't use any of these.

---

## Summary

| # | Item | Verdict | Effort |
|---|------|---------|--------|
| 1 | `render()` signature | **Do it** — mechanical 1-line fix | Minutes |
| 2 | Non-const render systems | **Do it** — 3 systems, cleaner code | 30 min |
| 3 | `font_helper.h` dispatch | **Do it** — verify compile, likely zero changes | Minutes |
| 4 | SINGLETON_FWD ODR fix | **Do it** — free correctness fix | Zero |
| 5 | ENTITY_ID_GEN ODR fix | **Do it** — free correctness fix | Zero |
| 6 | `consume_press` fix | **Do it** — free input bugfix | Zero |
| 7 | macOS mouse fix | **Do it** — free bugfix | Zero |
| 8 | ECS perf optimizations | **Do it** — free performance | Zero |
| 9 | `should_iterate()` | **Skip for now** — nice but not needed | 1-2 hours |
| 10 | Iteration hooks | **Skip** — no clear use case | N/A |
| 11 | KeyChord input | **Skip** — pharmasea has own input system, no need for modifier combos | Large refactor |
| 12 | `add_filter(lambda)` | **Use for new code only** — don't migrate existing queries | Zero |
| 13 | Auto gamepad mappings | **Skip** — path resolution risk, manual loading works | N/A |
| 14 | 3D drawing helpers | **Skip** — use raylib directly when needed | N/A |
| 15 | UI entity collection | **Skip** — pharmasea doesn't use afterhours UI entities | N/A |

---

## Decisions

| # | Question | Answer |
|---|----------|--------|
| Q1 | How to fix `render_entities` const-ness? | **C) `const_cast` now, clean up signatures later** (separate commit) |
| Q2 | Render system approach? | **A) Define `AFTERHOURS_SINGLE_RENDER_PASS` + switch to mutable overloads** (separate commit from Q1) |
| Q3 | Clean up `should_iterate()` systems? | **A) Yes**, add `should_iterate() { return false; }` to 6 systems (separate commit) |
| Q4 | `add_filter(lambda)` vs `add_mod()`? | **A) Lambda for new code**, leave existing `add_mod()`. TODO to migrate later. |
| Q5 | Pin strategy? | **Stay on main, no pin.** Just track HEAD. |

---

## Migration steps (commit plan)

**Commit 1: `be - upgrade vendor/afterhours to main`**
1. `cd vendor/afterhours && git checkout origin/main`
2. Update `.gitmodules` to track `main` branch
3. `const_cast` in `render_entities` to fix compile
4. Add `-DAFTERHOURS_SINGLE_RENDER_PASS` to `FLAGS` in makefile
5. Verify `font_helper.h` backend dispatch resolves correctly
6. Build and fix any other compile issues
7. Run `./pharmasea.exe -t`

**Commit 2: `be - switch render systems to mutable overloads`**
1. Update 3 render systems to use `for_each_with(Entity&, ..., float)` instead of const
2. Drop `mutable` from `RenderEntitySystem::debug_mode_on`
3. Build and verify

**Commit 3: `be - add should_iterate to once-only systems`**
1. Add `bool should_iterate() const override { return false; }` to 6 systems that only use `once()`
2. Build and verify

**Commit 4 (later): `be - clean up render_entities const signatures`**
1. Remove `const` from `render_entities` and `render_ui`
2. Remove `mutable` from `SystemManager systems` and `input_systems` members
3. Remove `const_cast` from `render_entities` and `process_inputs`
4. Propagate signature changes to callers

**TODO (backlog): migrate `add_mod()` queries to `add_filter(lambda)`**
- 25 call sites across `entity_query.h` and `entity_query.cpp`
