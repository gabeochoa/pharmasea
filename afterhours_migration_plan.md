# Afterhours Migration Plan

## Overview
Migrate pharmasea from duplicate ECS code to afterhours library. Follow pattern from kart-afterhours and my_name_chef: extend `afterhours::EntityQuery<EQ>` for custom queries, use `afterhours::SystemManager` directly, and migrate to afterhours plugins.

## Phase 0: Serialization Compatibility Review (CRITICAL - Do First)

**Purpose**: Verify afterhours serialization is compatible with pharmasea's network serialization before migrating core ECS.

**Current State**:
- Pharmasea has custom `bitsery::serialize` for `Entity` in `src/entity.h` (lines 40-58) that serializes: `id`, `entity_type`, `componentSet`, `cleanup`, and loops through `componentArray`
- Pharmasea has custom serializers for `RefEntity` and `OptEntity` in `src/entity.h` (lines 65-75)
- `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` is already defined in `src/ah.h`
- Components use polymorphic serialization via `MyPolymorphicClasses` in `src/network/polymorphic_components.h`
- `BaseComponent` extends `afterhours::BaseComponent` and adds `parent` pointer serialization

**Compatibility Issues to Resolve**:

1. **Entity Serialization Conflict**:
   - Need to verify if afterhours provides Entity serialization when `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` is set
   - Compare afterhours format with pharmasea's custom format (id, entity_type, componentSet, cleanup, componentArray loop)
   - **Action**: Search afterhours codebase for serialization implementation. If formats differ, decide: use afterhours (migrate data) OR keep custom (ensure compatibility with afterhours Entity structure)

2. **Component Serialization**:
   - Pharmasea uses `PolymorphicBaseClass<BaseComponent>` and `PolymorphicBaseClass<afterhours::BaseComponent>` pointing to same derived classes
   - `BaseComponent` adds `parent` pointer that needs serialization
   - **Action**: Verify afterhours component serialization doesn't conflict with `parent` field. May need to keep custom `BaseComponent::serialize()`

3. **Tags Serialization**:
   - Afterhours Entity has `TagBitset tags{}` field
   - Pharmasea custom serializer doesn't serialize tags
   - **Action**: Add tags serialization to custom serializer OR verify afterhours serializer handles it

4. **RefEntity/OptEntity Serialization**:
   - Pharmasea has custom serializers for these types
   - **Action**: Check if afterhours provides these. If not, keep custom serializers

**Verification Steps**:
1. Search afterhours codebase for `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` usage to find serialization implementation
2. Compare afterhours Entity serialization format with pharmasea's custom format
3. Test roundtrip serialization: serialize entity → deserialize → verify all fields match
4. Test network serialization: client → server → client roundtrip
5. Verify polymorphic component serialization still works with afterhours BaseComponent

**Migration Strategy**:
- **Option A**: If afterhours provides compatible serialization, remove custom serializers and use afterhours
- **Option B**: If formats are incompatible, keep custom serializers but ensure they work with afterhours Entity structure (add tags support)
- **Option C**: Hybrid - use afterhours for Entity structure, keep custom for game-specific fields (entity_type, etc.)

**Files to Review**:
- `vendor/afterhours/src/core/entity.h` - check for `#ifdef ENABLE_AFTERHOURS_BITSERY_SERIALIZE` blocks
- `src/entity.h` - custom Entity serializer (lines 40-58)
- `src/components/base_component.h` - custom BaseComponent with parent pointer
- `src/network/polymorphic_components.h` - polymorphic class registrations
- `src/network/shared.h` - serialization context setup (lines 259-297)

**Testing Requirements**:
- Create test that serializes/deserializes Entity with all component types
- Test network packet roundtrip (client → server → client)
- Verify entity_type field is preserved (game-specific, not in afterhours)
- Verify tags are serialized if using afterhours serializer
- Test polymorphic component deserialization (AIComponent, CanBeHeld hierarchies)

**Outcome**: Document compatibility decision and update Phase 1 plan accordingly.

## Phase 1: Core ECS Migration

### 1.1 Replace EntityQuery with afterhours::EntityQuery<EQ> pattern
**Files**: `src/entity_query.h`, `src/entity_query.cpp`, all files using `EntityQuery`

**Steps**:
- Create `src/query.h` extending `afterhours::EntityQuery<EQ>` (CRTP pattern)
- Migrate custom query methods from `src/entity_query.h`:
  - `whereInRange` → keep as custom method in EQ
  - `whereType` → migrate to use tags or component checks
  - `whereIsHoldingAnyFurniture` → keep as custom method
  - `whereCanPathfindTo` → keep as custom method
  - `orderByDist` → keep as custom method
- Replace all `EntityQuery()` usage with `EQ()` throughout codebase
- Remove `src/entity_query.h` and `src/entity_query.cpp` after migration

**Reference**: See `kart-afterhours/src/query.h` and `my_name_chef/src/query.h` for pattern

### 1.2 Migrate SystemManager to afterhours::SystemManager
**Files**: `src/system/system_manager.h`, `src/system/system_manager.cpp`, `src/game.cpp`

**Steps**:
- Replace `SystemManager` singleton with `afterhours::SystemManager` instance
- Convert system registration from manual loops to `systems.register_update_system()`, `systems.register_render_system()`
- Migrate `update_all_entities()`, `render_entities()`, `render_ui()` to use afterhours system pattern
- Update `src/game.cpp` to create and use `afterhours::SystemManager` instance
- Keep game-specific logic (state management, timing) but use afterhours systems for entity iteration

**Reference**: See `kart-afterhours/src/main.cpp` and `my_name_chef/src/main.cpp` for SystemManager usage

### 1.3 Migrate EntityHelper to afterhours::EntityHelper
**Files**: `src/entity_helper.h`, `src/entity_helper.cpp`, all files using `EntityHelper`

**Steps**:
- Replace `EntityHelper::get_entities()` with `afterhours::EntityHelper::get_entities()`
- Replace `EntityHelper::createEntity()` with `afterhours::EntityHelper::createEntity()`
- Migrate `createPermanentEntity()` to use tags (e.g., `entity.enableTag(PermanentTag)`)
- Replace `EntityHelper::cleanup()` with afterhours cleanup mechanisms
- Keep game-specific helpers (named entities, pathfinding cache) as wrapper functions
- Update all includes to use `afterhours::EntityHelper` directly

**Note**: Game-specific functionality like `getNamedEntity`, `isWalkable` should remain as wrapper functions

## Phase 2: Plugin Migration

### 2.1 Migrate Window/Resolution Management to window_manager plugin
**Files**: `src/engine/resolution.h`, `src/engine/app.cpp`, `src/engine/settings.cpp`

**Steps**:
- Include `afterhours/plugins/window_manager.h`
- Replace `rez::ResolutionExplorer` with `afterhours::window_manager::ProvidesAvailableWindowResolutions`
- Replace manual resolution tracking with `afterhours::window_manager::ProvidesCurrentResolution`
- Use `afterhours::window_manager::add_singleton_components()` in startup
- Register `afterhours::window_manager::register_update_systems()` in SystemManager
- Update `App::onWindowResize()` to use `window_manager::set_window_size()`
- Remove `src/engine/resolution.h` after migration

### 2.2 Migrate Input System to input_system plugin
**Files**: `src/engine/keymap.h`, `src/engine/keymap.cpp`, `src/system/input_process_manager.cpp`

**Steps**:
- Include `afterhours/plugins/input_system.h`
- Create entity with `afterhours::input::InputCollector` component
- Replace `KeyMap::is_event()` calls with `afterhours::input::is_action_pressed()`
- Migrate input mapping to `afterhours::input::ProvidesInputMapping`
- Register `afterhours::input::InputSystem` in SystemManager
- Keep game-specific input processing logic but use afterhours for raw input collection
- Consider keeping `KeyMap` as a wrapper for game-specific input actions if needed

**Note**: This may require significant refactoring of input handling - evaluate if full migration is worth it vs keeping KeyMap as adapter

### 2.3 Evaluate UI Plugin Migration
**Files**: `src/engine/ui/`, `src/layers/base_game_renderer.h`

**Steps**:
- Review `afterhours/plugins/ui/` capabilities vs current UI system
- If migrating: replace `ui::UIContext` with afterhours UI components
- Register `afterhours::ui::register_render_systems()` and update/render hooks
- This is likely the most complex migration - may want to defer or keep custom UI

### 2.4 Migrate Autolayout to afterhours::ui::autolayout
**Files**: `src/engine/ui/autolayout.h`, `src/engine/ui/widget.h`

**Current State**:
- Pharmasea has `src/engine/ui/autolayout.h` which appears to be commented out or incomplete (lines 8-514 are commented)
- Afterhours has full autolayout system in `afterhours/plugins/autolayout.h` with flexbox-like layout

**Steps**:
- Review afterhours autolayout API and capabilities
- Compare with pharmasea's commented autolayout implementation
- If afterhours provides needed features: migrate to `afterhours::ui::AutoLayout`
- Replace `Widget`-based layout with `afterhours::ui::UIComponent`-based layout
- Update UI rendering to use afterhours autolayout computed positions
- Remove or archive `src/engine/ui/autolayout.h` if fully replaced

**Note**: This could significantly simplify UI layout code if afterhours autolayout meets requirements

### 2.5 Evaluate Animation Plugin for UI Animations
**Files**: `src/engine/anim_library.h`, UI animation code

**Current State**:
- Pharmasea has `AnimLibrary` for 3D model animations (raymlib ModelAnimation)
- Afterhours has `animation` plugin for 2D value animations (UI, easing, sequences)
- Pharmasea uses `reasings.h` for easing functions

**Steps**:
- Evaluate if afterhours animation plugin can replace UI animation code
- Use `afterhours::animation::anim()` for UI transitions, fades, slides
- Keep `AnimLibrary` for 3D model animations (different use case)
- Replace direct easing function calls with afterhours animation system where applicable
- Register `afterhours::animation::register_update_systems<UIAnimationKey>()` in SystemManager

**Note**: Afterhours animation is for 2D/value animations, not 3D model animations - these serve different purposes

### 2.6 Migrate Library Pattern to afterhours::Library
**Files**: `src/engine/library.h`, all `*Library` classes

**Current State**:
- Pharmasea has custom `Library<T>` template in `src/engine/library.h`
- Afterhours has `Library<T>` in `vendor/afterhours/src/library.h` with better error handling (`tl::expected`)
- Multiple libraries use pharmasea version: `TextureLibrary`, `FontLibrary`, `AnimLibrary`, `ModelLibrary`, `SoundLibrary`, `MusicLibrary`, `ShaderLibrary`

**Steps**:
- Compare pharmasea `Library<T>` with afterhours `Library<T>`
- Migrate to afterhours `Library<T>` for better error handling
- Update all library implementations to use afterhours version:
  - `TextureLibrary` → use afterhours Library
  - `FontLibrary` → use afterhours Library  
  - `AnimLibrary` → use afterhours Library
  - `ModelLibrary` → use afterhours Library
  - `SoundLibrary` → use afterhours Library
  - `MusicLibrary` → use afterhours Library
  - `ShaderLibrary` → use afterhours Library
- Replace `Library<T>::Error` with afterhours error handling
- Remove `src/engine/library.h` after migration

**Benefits**: Better error handling, consistent API, less code to maintain

### 2.7 Evaluate Texture Manager Plugin for Sprite Rendering
**Files**: `src/engine/texture_library.h`, sprite rendering code

**Current State**:
- Pharmasea has `TextureLibrary` for texture loading
- Afterhours has `texture_manager` plugin with sprite components (`HasSpritesheet`, `HasSprite`, `HasAnimation`)
- Afterhours provides sprite rendering systems (`RenderSprites`, `RenderAnimation`)

**Steps**:
- Evaluate if afterhours sprite system fits pharmasea's rendering needs
- If using 2D sprites: migrate to `afterhours::texture_manager::HasSprite` components
- Use `afterhours::texture_manager::RenderSprites` system for sprite rendering
- Keep `TextureLibrary` for general texture loading if needed
- Consider using afterhours sprite animation for 2D animations

**Note**: Only migrate if pharmasea uses sprite-based 2D rendering. 3D model rendering should stay custom.

### 2.8 Use afterhours Font Helper Utilities
**Files**: `src/engine/font_util.h`, `src/engine/font_library.h`, font loading code

**Current State**:
- Pharmasea has `FontLibrary` and `font_util.h` for font management
- Afterhours has `font_helper.h` with font loading utilities and text measurement

**Steps**:
- Use `afterhours::load_font_from_file_with_codepoints()` for CJK font loading
- Use `afterhours::measure_text()` and `measure_text_utf8()` for text measurement
- Keep `FontLibrary` wrapper but use afterhours font helpers internally
- Evaluate if afterhours font helpers can replace `font_util.h` functions

**Note**: Afterhours font helpers are utilities, not a full library replacement - use alongside FontLibrary

### 2.9 Use afterhours Color Utilities
**Files**: Color usage throughout codebase

**Current State**:
- Pharmasea uses raylib colors directly
- Afterhours has `color` plugin with utilities: `darken()`, `increase()`, `set_opacity()`, `opacity_pct()`
- Afterhours has `HasColor` component for dynamic colors

**Steps**:
- Use `afterhours::colors::darken()`, `increase()`, `set_opacity()`, `opacity_pct()` utilities
- Consider `afterhours::HasColor` component for entities with dynamic colors
- Replace manual color manipulation with afterhours utilities
- Use `afterhours::colors` namespace constants where applicable

**Note**: Low-priority quality-of-life improvement, not critical migration

### 2.10 Migrate Bitset Utils to afterhours::bitset_utils
**Files**: `src/engine/bitset_utils.h`, all files using `bitset_utils::`

**Current State**:
- Pharmasea has `src/engine/bitset_utils.h` with enum-based bitset operations
- Afterhours has `bitset_utils.h` with similar functionality plus additional utilities
- Both have: `set()`, `reset()`, `test()`, `index_of_nth_set_bit()`, `get_random_enabled_bit()`, `get_first_enabled_bit()`
- Afterhours has additional: `get_next_boolean_bit()`, `get_random_disabled_bit()`, `get_first_disabled_bit()`, `get_next_disabled_bit()`
- Pharmasea has `for_each_enabled_bit()` which afterhours doesn't have

**Steps**:
- Compare functionality - afterhours version is more complete
- Migrate to use `afterhours::bitset_utils` namespace
- Replace pharmasea `bitset_utils::` calls with `afterhours::bitset_utils::`
- Keep or port `for_each_enabled_bit()` if needed (or use afterhours pattern)
- Remove `src/engine/bitset_utils.h` after migration
- Update all includes from `engine/bitset_utils.h` to `afterhours/bitset_utils.h` (via `ah.h`)

**Files Using Bitset Utils**:
- `src/dataclass/configdata.h` - uses `bitset_utils::test()`, `bitset_utils::ForEachFlow`
- Any other files using bitset utilities

**Note**: Afterhours version requires `std::mt19937&` generator parameter for random functions, may need to adapt pharmasea's `RandomEngine` integration

### 2.11 Use afterhours Drawing Helpers
**Files**: Drawing code throughout codebase

**Current State**:
- Pharmasea uses raylib drawing functions directly
- Afterhours has `drawing_helpers.h` with utilities: `draw_text_ex()`, `draw_text()`, `draw_rectangle()`, `draw_rectangle_outline()`, `draw_rectangle_rounded()` with per-corner rounding

**Steps**:
- Use `afterhours::draw_rectangle_rounded()` for rounded rectangles with per-corner control
- Use `afterhours::draw_text_ex()` and `draw_text()` for consistent text rendering
- Use `afterhours::draw_rectangle()` and `draw_rectangle_outline()` utilities
- Replace direct raylib calls with afterhours helpers where applicable
- Benefit: Per-corner rounded rectangles, consistent API

**Note**: Low-priority quality improvement, but rounded rectangle per-corner control is useful for UI

### 2.12 Standardize Afterhours Includes
**Files**: All files including afterhours headers

**Current State**:
- Some files include `afterhours/ah.h` directly
- Some files include afterhours internal headers: `afterhours/src/type_name.h`, `afterhours/src/singleton.h`, `afterhours/src/library.h`
- `src/ah.h` wrapper exists but not all files use it
- `src/engine/type_name.h` and `src/engine/singleton.h` forward to afterhours (good pattern)

**Steps**:
- Replace all `#include "afterhours/ah.h"` with `#include "ah.h"` (use wrapper)
- Replace all `#include "../../vendor/afterhours/src/..."` with `#include "ah.h"` or appropriate wrapper
- Update files that include internal headers:
  - `src/engine/library.h` - should use `ah.h` wrapper instead of direct include
  - Any other files including `afterhours/src/` or `afterhours/core/` directly
- Ensure `src/ah.h` wrapper sets all necessary flags (`ENABLE_AFTERHOURS_BITSERY_SERIALIZE`, `AFTER_HOURS_REPLACE_LOGGING`, etc.)
- Document include policy: always use `ah.h` wrapper, never include afterhours internal headers directly

**Files to Update**:
- `src/entity.h` - already uses `afterhours/ah.h`, change to `ah.h`
- `src/job.h` - already uses `afterhours/ah.h`, change to `ah.h`
- `src/components/base_component.h` - already uses `afterhours/ah.h`, change to `ah.h`
- `src/layers/gamelayer.h` - already uses `afterhours/ah.h`, change to `ah.h`
- `src/engine/library.h` - includes `afterhours/src/library.h` directly, change to use wrapper

**Benefits**: Centralized configuration, easier maintenance, ensures all necessary flags are set

## Implementation Notes

### Serialization
- Ensure `ENABLE_AFTERHOURS_BITSERY_SERIALIZE` is set (already in `src/ah.h`)
- After Phase 0 review: decide whether to use afterhours serialization or keep custom
- If keeping custom: add tags serialization support
- Test network serialization after each phase

### Tags Migration
- Replace `include_store_entities()` flag with entity tags
- Replace permanent entity tracking with tags
- Use `afterhours::EntityQuery::whereHasTag()` instead of custom filters

### Testing Strategy
- Each commit should build and run
- Test network serialization after Phase 0 and Phase 1
- Test input handling after Phase 2.2
- Test resolution changes after Phase 2.1

## Files to Modify

**Phase 0**:
- `vendor/afterhours/src/core/entity.h` (review serialization)
- `src/entity.h` (review/update custom Entity serializer)
- `src/components/base_component.h` (verify parent pointer serialization)
- `src/network/polymorphic_components.h` (verify polymorphic registrations)
- `src/network/shared.h` (verify serialization context)

**Phase 1**:
- `src/query.h` (new)
- `src/entity_query.h` (remove after migration)
- `src/system/system_manager.h` (refactor)
- `src/system/system_manager.cpp` (refactor)
- `src/entity_helper.h` (refactor)
- `src/entity_helper.cpp` (refactor)
- `src/game.cpp` (update SystemManager usage)
- All files using `EntityQuery` or `EntityHelper`

**Phase 2**:
- `src/engine/resolution.h` (remove after migration)
- `src/engine/app.cpp` (use window_manager)
- `src/engine/settings.cpp` (use window_manager)
- `src/engine/keymap.h` (evaluate migration)
- `src/system/input_process_manager.cpp` (use input plugin)
- `src/engine/ui/autolayout.h` (evaluate/remove if using afterhours)
- `src/engine/ui/widget.h` (evaluate if using afterhours UI)
- `src/engine/library.h` (remove after migration to afterhours Library)
- `src/engine/texture_library.h` (evaluate sprite migration)
- `src/engine/font_util.h` (use afterhours font helpers)
- `src/engine/bitset_utils.h` (remove after migration)
- All files using `bitset_utils::` (update to `afterhours::bitset_utils::`)
- Drawing code (use afterhours drawing helpers)

## Dependencies
- Phase 0 must complete before Phase 1 (serialization compatibility is critical)
- Phase 1.1 must complete before 1.2 and 1.3 (EntityQuery is used everywhere)
- Phase 1.2 and 1.3 can proceed in parallel
- Phase 2 can proceed after Phase 1 is stable
- Phase 2.6 (Library migration) should be done early as it affects many systems
- Phase 2.10 (Bitset Utils) should be done early as it's used in multiple places
- Phase 2.4 (Autolayout) can be done independently if UI migration proceeds
- Phase 2.5 (Animation), 2.9 (Color), 2.11 (Drawing Helpers) are low-priority quality improvements

## Phase 3: Code Health & Robustness Improvements

**Purpose**: Reduce fragility, complexity, and improve robustness based on codebase review and todo.md analysis.

### 3.1 Error Handling & Validation

**Issues Found**:
- Network deserialization uses `assert()` which crashes in release builds (`src/network/shared.h:296`)
- Minimal validation on deserialized packets (TODO comment at line 310)
- Error handling often just logs without graceful degradation
- 568 TODO/FIXME/HACK comments indicate incomplete error handling

**Actions**:
- Replace `assert(des.adapter().error() == ...)` with proper error handling that returns error codes or uses `tl::expected`
- Add validation layer for network packets: check packet size, type ranges, component validity
- Use `tl::expected<Entity, DeserializeError>` for deserialization instead of void functions
- Add bounds checking for all array/vector accesses
- Replace fatal asserts with recoverable error paths where possible
- Create `NetworkValidation` helper class for packet validation

**Files**:
- `src/network/shared.h` - replace assert with error handling
- `src/network/internal/client.cpp` - add validation before processing messages
- `src/network/internal/server.h` - add validation before processing messages
- `src/entity_helper.cpp` - add null checks and validation

### 3.2 Memory Safety & Resource Management

**Issues Found**:
- Mix of `shared_ptr`, `unique_ptr`, and raw `new`/`delete`
- Entity creation uses `new Entity()` instead of `make_shared` (`src/entity_helper.cpp:91`)
- No RAII wrappers for raylib resources (Model/Texture/Sound) - potential leaks
- Global entity vectors (`client_entities_DO_NOT_USE`, `server_entities_DO_NOT_USE`) with unclear ownership

**Actions**:
- Replace `new Entity()` with `std::make_shared<Entity>()` in `EntityHelper::createEntityWithOptions`
- Create RAII wrappers for raylib resources:
  - `RaylibModel` - auto-unloads in destructor
  - `RaylibTexture` - auto-unloads in destructor
  - `RaylibSound` - auto-unloads in destructor
- Audit all `new`/`delete` usage and convert to smart pointers
- Document ownership semantics for entity storage
- Use `gsl::not_null` for non-owning pointers that must not be null

**Files**:
- `src/entity_helper.cpp` - use make_shared
- `src/engine/model_library.h` - add RAII wrappers
- `src/engine/sound_library.h` - add RAII wrappers
- `src/preload.cpp` - use RAII wrappers

### 3.3 Type Safety & Strong Types

**Issues Found**:
- Heavy use of naked `int`/`float` parameters
- String-based global registry (`GLOBALS`) with no type safety
- EntityID, PlayerID are just `int` - easy to mix up
- No strong typedefs for IDs

**Actions**:
- Create strong type wrappers:
  - `struct EntityId { int value; };` with comparison operators
  - `struct PlayerId { int value; };`
  - `struct ComponentId { size_t value; };`
- Replace `int entity_id` parameters with `EntityId`
- Use `std::chrono::duration` for time values instead of raw `float dt`
- Consider typed enum for `EntityType` instead of `int entity_type`
- Create type-safe wrapper for `GLOBALS` registry (template-based, type-checked)

**Files**:
- `src/entity.h` - add EntityId type
- `src/entity_helper.h` - use EntityId
- `src/network/packet_types.h` - use strong types
- `src/engine/globals_register.h` - add type-safe wrapper

### 3.4 Network Robustness

**Issues Found**:
- Deserialization errors cause crashes (assert in release)
- No validation on packet size or structure
- TODO comment indicates missing validation (`src/network/shared.h:310`)
- No fuzz testing for malformed inputs
- Error codes logged but not handled

**Actions**:
- Add packet size validation before deserialization
- Validate packet type enum ranges
- Add checksum/version validation for critical packets
- Return error codes instead of asserting
- Create fuzz tests for network deserialization
- Add rate limiting for packet processing
- Validate component data ranges (e.g., position bounds, health > 0)

**Files**:
- `src/network/shared.h` - add validation functions
- `src/network/internal/client.cpp` - validate before processing
- `src/network/internal/server.h` - validate before processing
- `src/tests/` - add fuzz tests

### 3.5 Global State Reduction

**Issues Found**:
- Heavy use of global variables: `GLOBALS`, `client_entities_DO_NOT_USE`, `server_entities_DO_NOT_USE`
- Multiple singleton patterns
- Thread safety concerns (comments about client/server threads)
- String-based global registry is error-prone

**Actions**:
- Pass context explicitly instead of using globals where possible
- Make singleton access thread-safe (use atomics or mutexes)
- Replace `GLOBALS` string registry with typed, compile-time checked system
- Document which globals are truly needed vs. can be passed as parameters
- Consider dependency injection for systems that need multiple globals

**Files**:
- `src/engine/globals_register.h` - add type safety, consider replacement
- `src/entity_helper.cpp` - reduce global entity vectors
- `src/system/system_manager.cpp` - pass context explicitly

### 3.6 Code Simplification

**Issues Found**:
- 568 TODO/FIXME/HACK comments indicate technical debt
- Duplicate serialization code (`src/network/client.cpp:75` vs `src/network/internal/client.cpp:109`)
- Large system files (system_manager.cpp is 3000+ lines)
- Complex nested conditionals in systems

**Actions**:
- Prioritize and address high-impact TODOs (network, serialization, error handling)
- Extract duplicate serialization into shared function
- Split large system files into smaller, focused modules
- Refactor complex conditionals into early returns or guard clauses
- Extract magic numbers into named constants
- Use strategy pattern for complex conditional logic

**Files**:
- `src/network/client.cpp` - deduplicate serialization
- `src/system/system_manager.cpp` - split into smaller modules
- `src/system/rendering_system.cpp` - extract complex logic

### 3.7 Testing & Validation Infrastructure

**Issues Found**:
- No unit tests for serialization/deserialization
- No fuzz tests for network inputs
- No validation helpers for bounds checking
- Missing crash handler in release builds

**Actions**:
- Add unit tests for Entity serialization roundtrips
- Add unit tests for component serialization
- Create fuzz tests for network packet deserialization
- Add `BoundsChecker` helper class for array/vector access
- Enable backward-cpp crash handler in release builds (with symbol stripping)
- Add property-based tests for serialization (test all component combinations)

**Files**:
- `src/tests/serialization_tests.h` (new)
- `src/tests/network_fuzz_tests.h` (new)
- `src/engine/bounds_checker.h` (new)
- `src/game.cpp` - enable crash handler

### 3.8 Build & Static Analysis

**Issues Found**:
- No sanitizers enabled in debug builds
- No clang-tidy configuration
- Warnings not treated as errors in CI
- No compile-time profiling

**Actions**:
- Enable ASan + UBSan in Debug builds
- Add TSan for network tests
- Configure clang-tidy with bugprone-*, performance-*, readability-* checks
- Add -Werror to CI builds
- Add -ftime-trace to profile compile times
- Use Include-What-You-Use (IWYU) to reduce transitive includes

**Files**:
- `makefile` - add sanitizer flags
- `.clang-tidy` (new) - configure checks
- CI configuration - add -Werror

### 3.9 Documentation & Code Style

**Issues Found**:
- No documented include policy
- No code style guide
- Inconsistent naming (some use snake_case, some camelCase)
- Missing API documentation

**Actions**:
- Document include policy (prefer forward declarations, include what you use)
- Create code style guide
- Standardize naming convention (choose one: snake_case or camelCase)
- Add Doxygen comments for public APIs
- Document ownership semantics for entities/components
- Add pre-commit hook for formatting and basic checks

**Files**:
- `docs/CODING_STYLE.md` (new)
- `docs/INCLUDE_POLICY.md` (new)
- `.pre-commit-config.yaml` (new)

### 3.10 Contract Enforcement & Assertions

**Issues Found**:
- Missing preconditions/postconditions on functions
- No validation of function contracts (null pointers, valid ranges, state requirements)
- Bugs discovered late instead of at contract violation point
- Inconsistent use of VALIDATE macro vs assert

**Actions**:
- Add preconditions to all public API functions:
  - Null pointer checks for required parameters
  - Range validation for indices/IDs
  - State validation (e.g., entity must have component before accessing)
  - Type validation (e.g., entity must be of expected type)
- Add postconditions for critical operations:
  - Verify entity creation succeeded
  - Verify component addition/removal state
  - Verify serialization roundtrip integrity
- Use `VALIDATE()` macro (from `engine/assert.h`) for recoverable contract violations
- Use `assert()` for invariants that should never fail in correct code
- Add contract checks to:
  - Entity access functions (must exist, must have component)
  - Component getters (must have component before get)
  - Network deserialization (validate structure before processing)
  - System update functions (validate entity state before processing)
  - Query operations (validate query parameters)
- Document contract requirements in function comments
- Enable assertions in Debug builds, consider keeping critical ones in Release

**Files**:
- `src/entity_helper.h` - add preconditions to get/create functions
- `src/entity.h` - add asserts to component getters (has component check)
- `src/entity_query.h` - validate query parameters
- `src/system/system_manager.cpp` - validate entity state before processing
- `src/network/shared.h` - add validation before deserialization
- `src/components/base_component.h` - validate parent pointer when used
- All component access functions - assert component exists before get

**Examples**:
```cpp
// EntityHelper::getEntityForID
Entity& getEntityForID(EntityID id) {
    VALIDATE(id >= 0, "EntityID must be non-negative");
    auto* entity = find_entity(id);
    VALIDATE(entity != nullptr, "Entity with id {} not found", id);
    return *entity;
}

// Entity::get<T>
template<typename T> T& get() {
    VALIDATE(has<T>(), "Entity {} does not have component {}", id, type_name<T>());
    return static_cast<T&>(*componentArray.at(components::get_type_id<T>()).get());
}

// Network deserialization
static tl::expected<Entity, DeserializeError> deserialize_to_entity(const std::string& msg) {
    VALIDATE(!msg.empty(), "Cannot deserialize empty message");
    VALIDATE(msg.size() < MAX_PACKET_SIZE, "Packet size {} exceeds maximum", msg.size());
    // ... deserialization ...
}
```

## Phase 3 Priority Order

1. **Critical (Do First)**: 3.1 Error Handling, 3.4 Network Robustness, 3.10 Contract Enforcement
2. **High Priority**: 3.2 Memory Safety, 3.3 Type Safety
3. **Medium Priority**: 3.5 Global State, 3.6 Code Simplification
4. **Low Priority**: 3.7 Testing Infrastructure, 3.8 Build Tools, 3.9 Documentation

## Integration with Afterhours Migration

- Phase 0 (Serialization Review) must complete before Phase 1
- Phase 2.12 (Standardize Includes) should be done first - ensures all files use wrapper correctly
- Phase 2.10 (Bitset Utils) should be done early as it's used in multiple places
- Phase 2.6 (Library migration) should be done early as it affects many systems
- Phase 3.1 (Error Handling) should be done alongside Phase 0 (Serialization Review)
- Phase 3.2 (Memory Safety) should be done before Phase 1 (EntityHelper migration)
- Phase 3.3 (Type Safety) can be done incrementally during Phase 1
- Phase 3.4 (Network Robustness) is critical and should be done early
- Phase 3.10 (Contract Enforcement) should be done early to catch issues during migration
- Phase 3.5 (Global State) can be done after Phase 1 completes

## Migration Summary

**Total Migration Opportunities Identified**:
- **Phase 0**: 1 critical review (Serialization Compatibility)
- **Phase 1**: 3 core ECS migrations (EntityQuery, SystemManager, EntityHelper)
- **Phase 2**: 9 plugin/utility migrations (Window Manager, Input, UI, Autolayout, Animation, Library, Texture Manager, Font Helpers, Color, Bitset Utils, Drawing Helpers, Include Standardization)
- **Phase 3**: 10 code health improvements (Error Handling, Memory Safety, Type Safety, Network Robustness, Global State, Code Simplification, Testing, Build Tools, Documentation, Contract Enforcement)

**Estimated Impact**:
- **Code Reduction**: Removing duplicate implementations (EntityQuery, EntityHelper, Library, Bitset Utils, Autolayout) could reduce codebase by ~2000-3000 lines
- **Maintenance**: Centralizing on afterhours reduces maintenance burden - fixes/improvements benefit all projects
- **Robustness**: Using battle-tested afterhours code improves stability
- **Features**: Access to afterhours plugins (animation, autolayout, UI) provides new capabilities

**Risk Assessment**:
- **Low Risk**: Phase 2.9 (Color), 2.11 (Drawing Helpers) - utility functions, easy to rollback
- **Medium Risk**: Phase 2.6 (Library), 2.10 (Bitset Utils) - affects many files but straightforward replacements
- **High Risk**: Phase 1 (Core ECS), Phase 2.2 (Input), Phase 2.3 (UI) - fundamental systems, requires thorough testing
- **Critical Risk**: Phase 0 (Serialization) - network compatibility is essential, must be verified before proceeding

