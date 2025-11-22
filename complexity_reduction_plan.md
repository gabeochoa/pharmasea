# Code Complexity Reduction Review
## (Post-Afterhours Migration)

**Assumptions**: After reviewing the afterhours migration plan, this review assumes:
- Phase 1: EntityQuery, SystemManager, and EntityHelper migrated to afterhours (addressing global state, query optimization)
- Phase 3: Error handling, memory safety, type safety, network robustness, and global state reduction addressed
- Afterhours migration handles code simplification for core ECS systems

**Focus**: This plan targets complexity issues that remain AFTER the afterhours migration, focusing on:
- Game-specific logic complexity
- File organization and structure  
- Long functions and spaghetti code in game logic
- Algorithmic complexity
- Code duplication in game-specific systems

---

## Organization: Simple to Complex by Folder

### 1. Simple Utility Files (Root `src/`)

#### `vec_util.h`
- **Issue**: Mixed concerns - contains both vector utilities and neighbor iteration logic
- **Complexity**: Low
- **Refactor**: 
  - Extract neighbor iteration (`forEachNeighbor`, `get_neighbors`) into separate `neighbor_util.h`
  - Move `remove_if_matching` and `remove_all_matching` to a dedicated container utility file
  - Keep only pure vector math operations here

#### `text_util.h` 
- **Issue**: Very long file (373 lines) with multiple large functions doing similar work
- **Complexity**: Medium
- **Refactor**:
  - Extract `DrawTextCodepoint3D` (121 lines) into separate helper
  - Extract `MeasureText3D` and `MeasureTextWave3D` - they share 80% of logic
  - Create shared text measurement helper to reduce duplication
  - Split wave text logic into `text_wave_util.h`

#### `drawing_util.h`
- **Issue**: Long functions with repetitive vertex setup code
- **Complexity**: Low-Medium
- **Refactor**:
  - Extract vertex setup into helper functions for `DrawCubeCustom` (170 lines)
  - `DrawRect2Din3D` has TODO indicating it doesn't work - either fix or remove
  - Create vertex buffer helpers to reduce repetition

---

### 2. Components Folder (`src/components/`)

#### Component Organization
- **Issue**: 71 component files, many single-purpose but naming inconsistent
- **Complexity**: Low (individual files), Medium (as a system)
- **Refactor**:
  - Group related components (all `ai_*` components could be in `ai/` subfolder)
  - Group all `can_*` components together
  - Group all `has_*` components together
  - Group all `is_*` components together
  - This organization will help after afterhours migration when components are standardized

#### `has_dynamic_model_name.cpp` & `has_waiting_queue.cpp`
- **Issue**: Only 2 components have .cpp files, rest are header-only
- **Complexity**: Low
- **Refactor**: Consider if these need separate implementation files or can be header-only for consistency

---

### 3. Entity Makers (`src/entity_makers.*`)

**Note**: After afterhours migration, EntityHelper will be replaced, but entity creation logic remains game-specific.

#### `entity_makers.cpp`
- **Issue**: Massive file (1795 lines) with huge switch statement (lines 1353-1776)
- **Complexity**: Very High
- **Refactor**:
  - Split into multiple files: `furniture_makers.cpp`, `item_makers.cpp`, `entity_makers_base.cpp`
  - Replace switch statement with factory pattern or registry map:
    ```cpp
    // Instead of switch, use:
    static std::map<EntityType, std::function<void(Entity&, vec3, int)>> makers;
    // Register in init: makers[EntityType::Table] = furniture::make_table;
    ```
  - Extract common entity setup logic (lines 200-227, 244-251) into helper functions
  - `make_customer` function (lines 1430-1549) is 119 lines - split into:
    - `setup_customer_base()` - basic entity setup
    - `setup_customer_ai()` - AI components
    - `setup_customer_vomit_spawner()` - vomit spawner logic
  - `convert_to_type` switch (lines 1618-1794) should use same factory pattern

---

### 4. System Folder (`src/system/`)

**Note**: After afterhours migration, SystemManager will use afterhours::SystemManager, but game-specific update logic remains.

#### `system_manager.cpp` (Game-Specific Update Logic)
- **Issue**: Even after migration, game-specific update functions will remain (3000+ lines)
- **Complexity**: Very High
- **Refactor**:
  - Split game-specific update logic into focused files:
    - `system_updates_entity_position.cpp` - position updates, held items, furniture
    - `system_updates_conveyers.cpp` - conveyer belt logic
    - `system_updates_work.cpp` - work/progress systems
    - `system_updates_highlighting.cpp` - highlight system
  - Extract `get_new_held_position_custom` (lines 239-302) - complex switch with repetitive code:
    - Create `HeldPositionCalculator` class with strategy pattern for each positioner type
  - Extract `process_conveyer_items` - long function with nested conditionals:
    - Split into: `move_item_along_conveyer()`, `check_conveyer_destination()`, `transfer_to_destination()`
  - Group small update functions (lines 156-237) into logical modules

#### `ai_system.cpp`
- **Issue**: Long functions with complex nested logic in game-specific AI
- **Complexity**: High
- **Refactor**:
  - `validate_drink_order` (lines 25-113) has deeply nested upgrade checks:
    - Extract to `DrinkValidationStrategy` with separate validators for each upgrade
    - Use chain of responsibility pattern: `MocktailValidator`, `CantEvenTellValidator`, etc.
  - `process_ai_waitinqueue` (lines 168-283) is 115 lines:
    - Split into: `find_register()`, `wait_in_line()`, `receive_drink()`, `validate_order()`
  - Extract common AI pathfinding patterns into `ai_pathfinding_helpers.h`

#### `input_process_manager.cpp`
- **Issue**: 18 TODO comments indicate incomplete features
- **Complexity**: Medium-High
- **Refactor**: 
  - After afterhours input migration, remaining game-specific input logic should be organized
  - Extract complex input handling into separate handlers: `pickup_handler.cpp`, `drop_handler.cpp`, `interaction_handler.cpp`

#### `rendering_system.cpp`
- **Issue**: 24 TODO comments, likely complex rendering logic
- **Complexity**: Medium-High
- **Refactor**: Review after migration to identify remaining complexity

---

### 5. Layers Folder (`src/layers/`)

#### Layer Files
- **Issue**: Many small layer files, some with similar patterns
- **Complexity**: Low-Medium
- **Refactor**:
  - Extract common layer patterns into base class helpers
  - Some layers (like `gamedebuglayer.cpp`) might be too large - check line counts
  - Consider if some layers can share common update/render patterns

---

### 6. Network Folder (`src/network/`)

**Note**: After afterhours migration Phase 3.1 and 3.4, error handling and validation will be improved, but game-specific network logic remains.

#### Game-Specific Network Logic
- **Issue**: After migration, game-specific packet handling and business logic remains
- **Complexity**: Medium
- **Refactor**:
  - Organize game-specific packet handlers into separate files
  - Extract complex packet processing logic into focused functions
  - Group related network operations (player updates, entity sync, etc.)

---

### 7. Engine Folder (`src/engine/`)

**Note**: After afterhours migration, many utilities will be replaced, but game-specific engine code remains.

#### Remaining Engine Files
- **Issue**: After migration, some engine files may still have complexity
- **Complexity**: Medium
- **Refactor**:
  - Review files like `ui/ui.cpp` (13 TODO comments) after UI migration decisions
  - Organize remaining game-specific engine utilities

---

### 8. Main Game Files

#### `game.cpp`
- **Issue**: 22 TODO comments, startup function mixes concerns
- **Complexity**: Medium
- **Refactor**:
  - Extract initialization into separate functions: `init_window()`, `init_resources()`, `init_network()`
  - Group related TODOs and address systematically
  - After afterhours migration, startup will be simpler but game-specific init remains

#### `wave_collapse.cpp`
- **Issue**: Complex algorithm with long functions
- **Complexity**: High
- **Refactor**:
  - `run()` method (lines 235-274) orchestrates many steps:
    - Extract phases: `initial_cleanup_phase()`, `required_placement_phase()`, `wall_placement_phase()`, `wave_collapse_phase()`
  - `_place_pattern` (lines 96-148) does multiple things:
    - Split into: `collapse_cell()`, `update_neighbor_constraints()`, `remove_exhausted_patterns()`
  - Consider extracting pattern matching logic into `PatternMatcher` class
  - Extract validation logic into `PatternValidator` class

---

## Cross-Cutting Issues (Post-Migration)

### Code Duplication in Game Logic
- **Pattern**: Similar entity setup patterns in entity_makers
- **Files**: `entity_makers.cpp` (furniture setup, item setup)
- **Refactor**: Extract common setup patterns:
  - `setup_furniture_base()` - common furniture initialization
  - `setup_item_base()` - common item initialization
  - `setup_renderer()` - model/simple renderer setup

### Long Functions in Game Logic
- **Files**: `entity_makers.cpp`, `system_manager.cpp` (game-specific parts), `ai_system.cpp`
- **Issue**: Functions over 100 lines are common in game-specific logic
- **Refactor**: 
  - Extract helper functions aggressively
  - Use early returns and guard clauses
  - Split responsibilities (setup vs. logic vs. validation)

### Switch Statement Complexity
- **Files**: `entity_makers.cpp` (convert_to_type, make_item_type)
- **Issue**: Large switch statements that grow with each new type
- **Refactor**: 
  - Use factory pattern with registry:
    ```cpp
    class EntityFactory {
        std::map<EntityType, MakerFunc> makers;
    public:
        void register_maker(EntityType type, MakerFunc func);
        bool create(EntityType type, Entity& e, vec3 pos, int index);
    };
    ```

### Algorithmic Complexity
- **Files**: `wave_collapse.cpp`, `ai_system.cpp` (pathfinding)
- **Issue**: Some algorithms could be optimized or simplified
- **Refactor**:
  - Review wave collapse algorithm for optimization opportunities
  - Consider caching in pathfinding queries
  - Profile and optimize hot paths

### Component Interaction Complexity
- **Issue**: Complex interactions between components (e.g., CanHoldItem + ConveysHeldItem + CanBeTakenFrom)
- **Files**: `system_manager.cpp` (process_conveyer_items, etc.)
- **Refactor**:
  - Document component interaction patterns
  - Extract interaction logic into dedicated handler classes
  - Consider event system for component interactions

---

## Priority Recommendations (Post-Migration)

1. **High Priority**: Split `entity_makers.cpp` (1795 lines) - this is game-specific and won't be addressed by migration
2. **High Priority**: Refactor `system_manager.cpp` game-specific update logic into focused modules
3. **High Priority**: Replace switch statements in entity_makers with factory pattern
4. **Medium Priority**: Extract long functions in `ai_system.cpp` (validate_drink_order, process_ai_waitinqueue)
5. **Medium Priority**: Refactor `text_util.h` to reduce duplication
6. **Medium Priority**: Organize `wave_collapse.cpp` algorithm into clearer phases
7. **Low Priority**: Reorganize components folder structure
8. **Low Priority**: Extract common entity setup patterns to reduce duplication

---

## Implementation Todos

1. Split entity_makers.cpp (1795 lines) into furniture_makers.cpp, item_makers.cpp, and entity_makers_base.cpp. Replace large switch statements with factory pattern using registry map.

2. Split system_manager.cpp game-specific update logic into focused modules: system_updates_entity_position.cpp, system_updates_conveyers.cpp, system_updates_work.cpp, system_updates_highlighting.cpp

3. Extract get_new_held_position_custom into HeldPositionCalculator class with strategy pattern for each positioner type

4. Refactor validate_drink_order: extract upgrade validation logic to DrinkValidationStrategy with chain of responsibility (MocktailValidator, CantEvenTellValidator)

5. Split process_ai_waitinqueue (115 lines) into smaller functions: find_register(), wait_in_line(), receive_drink(), validate_order()

6. Refactor text_util.h: extract DrawTextCodepoint3D, create shared measurement helper for MeasureText3D/MeasureTextWave3D, split wave text logic

7. Refactor wave_collapse.cpp: extract run() phases (initial_cleanup, required_placement, wall_placement, wave_collapse), split _place_pattern responsibilities, extract PatternMatcher class

8. Extract common entity setup patterns: setup_furniture_base(), setup_item_base(), setup_renderer() to reduce duplication in entity_makers

9. Reorganize components folder: group ai_* in ai/, can_* together, has_* together, is_* together for better organization

10. Split process_conveyer_items into: move_item_along_conveyer(), check_conveyer_destination(), transfer_to_destination()

