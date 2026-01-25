# Hot-Reload System Design

## Executive Summary

This document describes the hot-reload system for PharmaScript, enabling live code updates without losing game state.

**Current Focus:** C++ hot-reload via dynamic library swapping. PharmaScript (custom language) is deferred for later.

**Key Design Decisions:**
- Hot-reload C++ system files directly via `.dylib` compilation
- ~500ms-2s reload time per changed file
- Systems first (components stay static to preserve memory layout)
- State serialization for the ~6% of systems that have member variables

---

## C++ Hot-Reload Architecture

### Overview

```
┌─────────────────────────────────────────────────────────────┐
│  Development Mode                                           │
├─────────────────────────────────────────────────────────────┤
│  1. File watcher detects .cpp/.h change                     │
│  2. Compile changed system to .dylib (~500ms-2s)            │
│  3. Serialize system state (if any)                         │
│  4. dlclose old dylib                                       │
│  5. dlopen new dylib                                        │
│  6. Call factory function to create new system              │
│  7. Restore system state                                    │
│  8. Replace system in SystemManager                         │
│  9. Continue execution                                      │
└─────────────────────────────────────────────────────────────┘
```

### System Factory Pattern

Each hot-reloadable system needs a C-linkage factory function:

```cpp
// src/system/hot/movement_system.h
#pragma once
#include "../ah.h"
#include "../../components/transform.h"
#include "../../components/velocity.h"

struct MovementSystem : public afterhours::System<Transform, Velocity> {
    void for_each_with(Entity& entity, Transform& pos, Velocity& vel, float dt) override {
        pos.x += vel.dx * dt;
        pos.y += vel.dy * dt;
    }
};

// Factory functions for hot-reload
extern "C" {
    afterhours::SystemBase* create_system() {
        return new MovementSystem();
    }

    void destroy_system(afterhours::SystemBase* sys) {
        delete sys;
    }

    const char* system_name() {
        return "MovementSystem";
    }
}
```

### Compilation Command

```bash
# Compile single system to dylib
clang++ -std=c++20 -O2 -shared -fPIC \
    -I./src -I./vendor/afterhours/src \
    src/system/hot/movement_system.cpp \
    -o build/hot/movement_system.dylib
```

### HotReloadManager

```cpp
// src/hot_reload/hot_reload_manager.h
#pragma once
#include <filesystem>
#include <unordered_map>
#include <dlfcn.h>

class HotReloadManager {
public:
    static HotReloadManager& get() {
        static HotReloadManager instance;
        return instance;
    }

    void init(const std::string& watch_dir);
    void update();  // Check for file changes, trigger recompile

private:
    struct LoadedSystem {
        void* handle;                    // dlopen handle
        afterhours::SystemBase* system;  // Created system instance
        std::filesystem::file_time_type last_modified;
        std::string source_path;
        std::string dylib_path;
    };

    std::unordered_map<std::string, LoadedSystem> systems_;
    std::string watch_dir_;

    void recompile_and_reload(const std::string& name);
    void serialize_state(LoadedSystem& sys, nlohmann::json& out);
    void deserialize_state(LoadedSystem& sys, const nlohmann::json& in);
};
```

### State Preservation

Only 4 systems (~6%) have state. They implement serialization:

```cpp
struct CountPlayersSystem : public afterhours::System<IsTriggerArea> {
    // State that needs preservation
    int count = 0;

    // Hot-reload interface
    nlohmann::json serialize_state() const {
        return {{"count", count}};
    }

    void deserialize_state(const nlohmann::json& j) {
        count = j.value("count", 0);
    }

    void for_each_with(Entity& entity, IsTriggerArea& ita, float dt) override {
        // ... use count
    }
};
```

### File Watcher

```cpp
void HotReloadManager::update() {
    for (auto& [name, sys] : systems_) {
        auto current_time = std::filesystem::last_write_time(sys.source_path);
        if (current_time > sys.last_modified) {
            recompile_and_reload(name);
            sys.last_modified = current_time;
        }
    }
}

void HotReloadManager::recompile_and_reload(const std::string& name) {
    auto& sys = systems_[name];

    // 1. Compile
    std::string cmd = fmt::format(
        "clang++ -std=c++20 -O2 -shared -fPIC "
        "-I./src -I./vendor/afterhours/src "
        "{} -o {}",
        sys.source_path, sys.dylib_path);

    int result = std::system(cmd.c_str());
    if (result != 0) {
        log_error("Compile failed for {}", name);
        return;
    }

    // 2. Serialize state
    nlohmann::json state;
    serialize_state(sys, state);

    // 3. Unload old
    if (sys.system) {
        auto destroy_fn = (void(*)(afterhours::SystemBase*))dlsym(sys.handle, "destroy_system");
        if (destroy_fn) destroy_fn(sys.system);
    }
    if (sys.handle) dlclose(sys.handle);

    // 4. Load new
    sys.handle = dlopen(sys.dylib_path.c_str(), RTLD_NOW);
    if (!sys.handle) {
        log_error("dlopen failed: {}", dlerror());
        return;
    }

    auto create_fn = (afterhours::SystemBase*(*)())dlsym(sys.handle, "create_system");
    if (!create_fn) {
        log_error("create_system not found");
        return;
    }

    sys.system = create_fn();

    // 5. Restore state
    deserialize_state(sys, state);

    // 6. Register with SystemManager
    SystemManager::get().replace_system(name, sys.system);

    log_info("Hot-reloaded: {}", name);
}
```

### Limitations

| Issue | Mitigation |
|-------|------------|
| Templates across dylib | Keep template instantiations in main binary, or use explicit instantiation |
| Header changes | May require full rebuild; hot-reload only for .cpp body changes |
| Component changes | Requires restart (memory layout changes) |
| Compile time | Use PCH, minimize includes |
| macOS code signing | May need to disable for dev builds |

### Directory Structure

```
src/
├── system/
│   ├── hot/                    # Hot-reloadable systems
│   │   ├── movement_system.h
│   │   ├── movement_system.cpp
│   │   ├── ai_movement.h
│   │   └── ai_movement.cpp
│   └── static/                 # Always compiled into main binary
│       ├── rendering_system.h
│       └── core_systems.h
├── hot_reload/
│   ├── hot_reload_manager.h
│   └── hot_reload_manager.cpp
build/
└── hot/                        # Compiled .dylib files
    ├── movement_system.dylib
    └── ai_movement.dylib
```

### Syntax Highlighting

Since we're using C++ directly, standard C++ syntax highlighting works out of the box in any editor.

If PharmaScript is implemented later, a VS Code extension would be needed:
```
pharmasea-phs/
├── package.json           # Extension manifest
├── syntaxes/
│   └── phs.tmLanguage.json  # TextMate grammar
└── language-configuration.json
```

---

## Implementation Checklist

- [ ] Create `src/hot_reload/` directory
- [ ] Implement `HotReloadManager` singleton
- [ ] Add `extern "C"` factory functions to one test system
- [ ] Test compile → dlopen → dlclose cycle
- [ ] Add file watcher polling in main loop
- [ ] Implement `SystemManager::replace_system()`
- [ ] Add state serialization interface to `SystemBase`
- [ ] Migrate one real system to `src/system/hot/`
- [ ] Test hot-reload with state preservation
- [ ] Add error overlay for compile failures

---

## Future: PharmaScript Language (Deferred)

The following sections document a potential custom scripting language for cleaner syntax. This is kept for future reference.

---

## PharmaScript Language Specification

### System Types

| Keyword | Component Access | Use Case |
|---------|------------------|----------|
| `const system` | Read-only (all components passed as `const&`) | Rendering, queries, calculations |
| `update system` | Read-write (all components passed as `&`) | Movement, damage, state changes |

### Query and Tag Filters

Systems filter entities using `query` (components) and `tags` (entity tags).

**Filter modifiers:**

| Syntax | Meaning | Generated C++ |
|--------|---------|---------------|
| `Component` | Entity must have component | `Component` |
| `!Component` | Entity must NOT have component | `afterhours::tags::Not<Component>` |
| `Tag` | Entity must have tag | `afterhours::tags::All<Tag>` |
| `!Tag` | Entity must NOT have tag | `afterhours::tags::Not<Tag>` |
| `any<A, B>` | Entity has A or B | `afterhours::tags::Any<A, B>` |

**Examples:**

```phs
// Only entities WITH IsPnumaticPipe, WITHOUT IsDisabled
update system ActivePipes {
    query: [IsPnumaticPipe, !IsDisabled];
    tags: [EntityType::PnumaticPipe];

    for_each(entity, pipe: IsPnumaticPipe, dt: f32) { ... }
}

// Entities that are Customer OR Employee
const system RenderPeople {
    query: [Transform, Health];
    tags: [any<EntityType::Customer, EntityType::Employee>];

    render(entity, transform: Transform, health: Health, dt: f32) { ... }
}

// Exclude entities marked for deletion
update system ProcessActive {
    query: [MyComponent];
    tags: [!MarkedForDeletion];

    for_each(entity, comp: MyComponent, dt: f32) { ... }
}
```

### Template Syntax (Pass-through)

C++ template syntax like `entity.get<Component>()` is passed through directly to generated code:

```phs
// PharmaScript
if (entity.is_missing<CanHoldItem>()) { return; }
CanHoldItem& chi = entity.get<CanHoldItem>();
OptEntity found = EntityQuery().whereHasComponent<IsPnumaticPipe>().gen_first();
```

```cpp
// Generated C++ (identical)
if (entity.is_missing<CanHoldItem>()) { return; }
CanHoldItem& chi = entity.get<CanHoldItem>();
OptEntity found = EntityQuery().whereHasComponent<IsPnumaticPipe>().gen_first();
```

The lexer distinguishes `<`/`>` as:
- **Comparison operators** when between expressions: `a < b`
- **Template delimiters** when after identifier: `foo<Bar>()`

### Basic Syntax

```phs
// Simple stateless update system
update system ProcessPnumaticPipeMovement {
    query: [IsPnumaticPipe];
    tags: [EntityType::PnumaticPipe];

    should_run() -> bool {
        if (!GameState::get().is_game_like()) { return false; }
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
        if (timer.needs_to_process_change) { return false; }
        return timer.is_bar_open();
    }

    for_each(entity, ipp: IsPnumaticPipe, dt: f32) {
        if (!ipp.has_pair()) { return; }

        OptEntity paired = EntityQuery()
            .whereID(ipp.paired.id)
            .whereHasComponent<IsPnumaticPipe>()
            .gen_first();

        if (!paired) { return; }

        const IsPnumaticPipe& other = paired->get<IsPnumaticPipe>();
        if (ipp.recieving == other.recieving) { return; }

        if (ipp.recieving) {
            // Transfer item from sender to receiver
            transfer_item_between_pipes(entity, paired.asE());
        }
    }
}
```

### System with State (Rare)

Only ~6% of systems need state. Use explicit `state {}` block:

```phs
update system CountAllPossibleTriggerAreaEntrants {
    state {
        count: i32 = 0;
    }

    query: [IsTriggerArea];

    once(dt: f32) {
        // Reset count at start of frame
        state.count = 0;
        for (player : EntityQuery().whereType(EntityType::Player).gen()) {
            state.count += 1;
        }
    }

    for_each(entity, ita: IsTriggerArea, dt: f32) {
        // Use cached count
        ita.possible_entrants = state.count;
    }
}
```

### Trigger/Callback Systems

Complex systems with switch-based dispatch:

```phs
update system TriggerCbOnFullProgress {
    query: [IsTriggerArea];
    tags: [TriggerTag::TriggerAreaFullNeedsProcessing];

    for_each(entity, ita: IsTriggerArea, dt: f32) {
        handle_progress_full(entity, ita);
        entity.disableTag(TriggerTag::TriggerAreaFullNeedsProcessing);
    }

    fn handle_progress_full(entity: Entity&, ita: IsTriggerArea&) {
        if (entity.hasTag(TriggerTag::GateTriggerWhileOccupied) &&
            ita.active_entrants() > 0) {
            if (entity.hasTag(TriggerTag::TriggerFiredWhileOccupied)) {
                return;
            }
            entity.enableTag(TriggerTag::TriggerFiredWhileOccupied);
        }

        ita.reset_cooldown();

        switch (ita.type) {
            IsTriggerArea::Store_Reroll => handle_store_reroll();
            IsTriggerArea::Lobby_PlayGame => handle_lobby_playgame();
            IsTriggerArea::Store_BackToPlanning => handle_store_back_to_planning();
            // ... other cases
        }
    }

    fn handle_store_reroll() {
        store::cleanup_old_store_options();
        store::generate_store_options();

        OptEntity sophie = EntityQuery().whereType(EntityType::Sophie).gen_first();
        if (!sophie.valid()) { return; }

        IsBank& bank = sophie->get<IsBank>();
        IsRoundSettingsManager& irsm = sophie->get<IsRoundSettingsManager>();
        i32 price = irsm.get<i32>(ConfigKey::StoreRerollPrice);
        bank.withdraw(price);
        irsm.config.permanently_modify(ConfigKey::StoreRerollPrice, Operation::Add, 25);
    }

    fn handle_lobby_playgame() {
        GameState::get().transition_to_game();
        move_all_players_to_state(game::State::InGame);
    }

    fn handle_store_back_to_planning() {
        store::move_purchased_furniture();
        GameState::get().transition_to_game();
        move_players_in_building_to_state(STORE_BUILDING, game::State::InGame);
    }

    fn move_all_players_to_state(state: game::State) {
        for (player : EQ(SystemManager::get().oldAll)
                        .whereType(EntityType::Player)
                        .gen()) {
            move_player_SERVER_ONLY(player, state);
        }
    }
}
```

### Read-Only Const Systems

```phs
const system RenderHealthBars {
    query: [Transform, Health];
    tags: [EntityType::Customer];

    render(entity, transform: Transform, health: Health, dt: f32) {
        vec2 pos = transform.as2();
        draw_health_bar(pos.x, pos.y - 1.0, health.ratio());
    }
}
```

---

## Migration Examples

### Example 1: ProcessPnumaticPipeMovementSystem (80 lines → 45 lines)

**Original C++ (`src/system/afterhours/inround/process_pnumatic_pipe_movement_system.h`):**
```cpp
struct ProcessPnumaticPipeMovementSystem
    : public afterhours::System<
          IsPnumaticPipe, afterhours::tags::All<EntityType::PnumaticPipe>> {
    virtual ~ProcessPnumaticPipeMovementSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp,
                               float) override {
        // ... 50 lines of logic
    }
};
```

**PharmaScript (`scripts/systems/process_pnumatic_pipe_movement.phs`):**
```phs
update system ProcessPnumaticPipeMovement {
    query: [IsPnumaticPipe];
    tags: [EntityType::PnumaticPipe];

    should_run() -> bool {
        if (!GameState::get().is_game_like()) { return false; }
        OptEntity sophie = EntityHelper::getPossibleNamedEntity(NamedEntity::Sophie);
        if (!sophie) { return false; }
        const HasDayNightTimer& timer = sophie->get<HasDayNightTimer>();
        if (timer.needs_to_process_change) { return false; }
        return timer.is_bar_open();
    }

    for_each(entity, ipp: IsPnumaticPipe, dt: f32) {
        if (!ipp.has_pair()) { return; }

        OptEntity paired = EntityQuery()
            .whereID(ipp.paired.id)
            .whereHasComponent<IsPnumaticPipe>()
            .gen_first();

        if (!paired) { return; }

        const IsPnumaticPipe& other = paired->get<IsPnumaticPipe>();

        // Skip if both same mode
        if (ipp.recieving == other.recieving) { return; }

        // We are receiving, they are sending
        if (ipp.recieving) {
            if (paired->is_missing<CanHoldItem>()) { return; }
            CanHoldItem& other_chi = paired->get<CanHoldItem>();
            if (other_chi.empty()) { return; }

            if (entity.is_missing<CanHoldItem>()) { return; }
            CanHoldItem& our_chi = entity.get<CanHoldItem>();
            if (our_chi.is_holding_item()) { return; }

            OptEntity item = other_chi.const_item();
            if (!item) {
                other_chi.update(nullptr, entity_id::INVALID);
                return;
            }

            if (!our_chi.can_hold(item.asE(), RespectFilter::ReqOnly)) { return; }

            // Transfer item
            our_chi.update(item.asE(), entity.id);
            other_chi.update(nullptr, entity_id::INVALID);
            entity.get<IsPnumaticPipe>().recieving = false;
        }
    }
}
```

### Example 2: AI Helper Functions (Standalone Functions)

**Original C++ (`src/system/ai/ai_system.cpp`):**
```cpp
namespace system_manager::ai {
[[nodiscard]] bool needs_bathroom_now(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return false;
    if (entity.is_missing<IsAIControlled>()) return false;
    // ...
}

float get_speed_for_entity(Entity& entity) {
    float base_speed = entity.get<HasBaseSpeed>().speed();
    if (entity.has<CanOrderDrink>()) {
        const CanOrderDrink& cod = entity.get<CanOrderDrink>();
        int denom = RandomEngine::get().get_int(1, max(1, cod.num_alcoholic_drinks_drank()));
        base_speed *= 1.0 / denom;
        base_speed = fmaxf(1.0, base_speed);
    }
    return base_speed;
}
}
```

**PharmaScript (`scripts/ai/ai_helpers.phs`):**
```phs
namespace ai {
    fn needs_bathroom_now(entity: Entity&) -> bool {
        if (entity.is_missing<CanOrderDrink>()) { return false; }
        if (entity.is_missing<IsAIControlled>()) { return false; }
        if (!entity.get<IsAIControlled>().has_ability(IsAIControlled::AbilityUseBathroom)) {
            return false;
        }

        if (entity.has<CanHoldItem>()) {
            if (!entity.get<CanHoldItem>().empty()) { return false; }
        }

        OptEntity sophie = EntityHelper::getPossibleNamedEntity(NamedEntity::Sophie);
        if (!sophie) { return false; }
        if (sophie->is_missing<IsRoundSettingsManager>()) { return false; }

        const IsRoundSettingsManager& irsm = sophie->get<IsRoundSettingsManager>();
        i32 bladder_size = irsm.get<i32>(ConfigKey::BladderSize);
        const CanOrderDrink& cod = entity.get<CanOrderDrink>();

        return cod.get_drinks_in_bladder() >= bladder_size;
    }

    fn get_speed_for_entity(entity: Entity&) -> f32 {
        f32 base_speed = entity.get<HasBaseSpeed>().speed();

        if (entity.has<CanOrderDrink>()) {
            const CanOrderDrink& cod = entity.get<CanOrderDrink>();
            i32 denom = RandomEngine::get().get_int(1, max(1, cod.num_alcoholic_drinks_drank()));
            base_speed *= 1.0 / denom;
            base_speed = fmaxf(1.0, base_speed);
        }

        return base_speed;
    }

    fn pick_random_walkable_near(entity: Entity&, attempts: i32) -> Optional<vec2> {
        const vec2 base = entity.get<Transform>().as2();
        for (i32 i = 0; i < attempts; i += 1) {
            vec2 off = RandomEngine::get().get_vec(-10.0, 10.0);
            vec2 p = base + off;
            if (EntityHelper::isWalkable(p)) { return p; }
        }
        return nullopt;
    }
}
```

---

## Type System

### Primitive Types

| PharmaScript | C++ Equivalent |
|--------------|----------------|
| `i8`, `i16`, `i32`, `i64` | `int8_t`, `int16_t`, `int32_t`, `int64_t` |
| `u8`, `u16`, `u32`, `u64` | `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` |
| `f32`, `f64` | `float`, `double` |
| `bool` | `bool` |
| `str` | `std::string` |

### Reference Types

| PharmaScript | C++ Equivalent |
|--------------|----------------|
| `T&` | `T&` (mutable reference) |
| `const T&` | `const T&` (immutable reference) |
| `T*` | `T*` (pointer) |
| `Optional<T>` | `std::optional<T>` |

### Passthrough Types

These types are passed through directly to C++:
- `Entity`, `OptEntity`, `RefEntity`
- `vec2`, `vec3`, `vec4`
- All component types (`Transform`, `Health`, etc.)
- All enum types (`EntityType`, `game::State`, etc.)

---

## Hot-Reload Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Development Mode                                           │
├─────────────────────────────────────────────────────────────┤
│  1. File watcher detects .phs change                        │
│  2. Transpile .phs → .cpp (< 10ms)                          │
│  3. Compile .cpp → .dylib (~200-400ms)                      │
│  4. Serialize system state (if any)                         │
│  5. Unload old dylib                                        │
│  6. Load new dylib                                          │
│  7. Restore system state                                    │
│  8. Continue execution                                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Release Mode                                               │
├─────────────────────────────────────────────────────────────┤
│  1. Transpile all .phs → .cpp at build time                 │
│  2. Compile into main executable                            │
│  3. Zero runtime overhead                                   │
└─────────────────────────────────────────────────────────────┘
```

### State Preservation

Systems with `state {}` blocks get automatic serialization:

```cpp
// Generated C++ for state preservation
struct ProcessPnumaticPipeMovement : System<...> {
    struct State {
        int32_t count = 0;
    } state;

    // Auto-generated
    nlohmann::json serialize_state() const {
        return {{"count", state.count}};
    }

    void deserialize_state(const nlohmann::json& j) {
        state.count = j.value("count", 0);
    }
};
```

### CLI Commands

The `phs` transpiler CLI supports multiple modes:

```bash
# Watch mode: hot-reload during development
phs watch scripts/
# Monitors .phs files, transpiles → compiles → hot-reloads on change

# Dump single file to C++
phs dump scripts/systems/movement.phs
# Outputs: build/generated/movement.cpp

# Dump all scripts to C++ directory
phs dump-all scripts/ --out src/generated/
# Generates all .cpp files, useful for debugging or ejecting

# Release build: generate C++ for static compilation
phs release scripts/ --out src/generated/
# Same as dump-all, but optimized for inclusion in main build

# Check syntax without generating
phs check scripts/systems/movement.phs

# Format a file
phs fmt scripts/systems/movement.phs
```

**Use cases:**

| Command | Purpose |
|---------|---------|
| `phs watch` | Development iteration with hot-reload |
| `phs dump` | Debug transpiler output, inspect generated C++ |
| `phs dump-all` | Eject from PharmaScript, commit C++ to repo permanently |
| `phs release` | Generate C++ for release builds (no runtime needed) |
| `phs check` | CI validation, catch syntax errors early |
| `phs fmt` | Consistent code formatting |

---

## Current Architecture Overview

### ECS Framework (AfterHours)

The codebase uses a custom Entity-Component-System architecture:

```
SystemManager::run(dt)
  → fixed_tick_all(dt)     // 1/120s fixed timestep
  → tick_all(dt)           // Variable timestep update
  → cleanup()              // Entity deletion
  → render_all(dt)         // Rendering pass
```

**Key Files:**
- `vendor/afterhours/src/core/system.h` - System base class and registration
- `vendor/afterhours/src/core/entity.h` - Entity storage with component arrays
- `vendor/afterhours/src/core/base_component.h` - Component base (max 128 components)

### System Registration Pattern

Systems are registered at startup via:
```cpp
afterhours::register_update_system<MySystem>();
afterhours::register_render_system<MyRenderSystem>();
```

Screen systems use static auto-registration:
```cpp
REGISTER_EXAMPLE_SCREEN(name, category, desc, SystemType)
```

### State That Must Be Preserved

| Data Type | Location | Preservation Difficulty |
|-----------|----------|------------------------|
| Settings (resolution, audio) | `Settings` singleton, JSON file | Easy - already serialized |
| Screen state (sliders, selections) | ScreenSystem member variables | Medium - needs serialization |
| Entity hierarchy | `EntityHelper::get_entities()` | Hard - dynamic, complex |
| Focus/input state | `UIContext<InputAction>` | Medium - single struct |
| Render textures | `mainRT`, `screenRT` | Easy - recreatable |

---

## Implementation Roadmap

### Phase 1: Transpiler Core

1. **Lexer** - Tokenize `.phs` files
   - Keywords: `const`, `update`, `system`, `query`, `tags`, `state`, `fn`, `for_each`, `once`, `after`, `render`
   - Types: `i32`, `f32`, `bool`, `str`, etc.
   - Operators, braces, semicolons

2. **Parser** - Build AST
   - System declarations
   - Function definitions
   - Expressions and statements

3. **Code Generator** - Emit C++
   - Generate struct inheriting from `afterhours::System<...>`
   - Wire up `for_each_with`, `should_run`, etc.
   - Generate state serialization if needed

### Phase 2: Hot-Reload Runtime

1. **File Watcher**
   - Monitor `scripts/` directory
   - Queue changed files for reload

2. **Compiler Driver**
   - Invoke transpiler
   - Compile to `.dylib` with clang++
   - Handle compile errors gracefully

3. **Dynamic Loader**
   - `dlopen`/`dlclose` for dylib management
   - Symbol lookup for system factory functions
   - State serialization/deserialization

### Phase 3: Developer Experience

1. **Error Reporting**
   - Source-mapped line numbers
   - Clear error messages

2. **Tooling**
   - Syntax highlighting (VS Code extension)
   - Basic LSP for autocomplete

---

## Appendix: Generated Code Example

### Input PharmaScript

```phs
update system ProcessPnumaticPipeMovement {
    query: [IsPnumaticPipe];
    tags: [EntityType::PnumaticPipe];

    should_run() -> bool {
        if (!GameState::get().is_game_like()) { return false; }
        return true;
    }

    for_each(entity, ipp: IsPnumaticPipe, dt: f32) {
        if (!ipp.has_pair()) { return; }
        // ... logic
    }
}
```

### Generated C++

```cpp
// Auto-generated from scripts/systems/process_pnumatic_pipe_movement.phs
#pragma once
#include "ah.h"
#include "components/is_pnumatic_pipe.h"

namespace scripted_systems {

struct ProcessPnumaticPipeMovement
    : public afterhours::System<
          IsPnumaticPipe,
          afterhours::tags::All<EntityType::PnumaticPipe>> {

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) { return false; }
        return true;
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp, float dt) override {
        if (!ipp.has_pair()) { return; }
        // ... logic
    }
};

// Factory for hot-reload
extern "C" afterhours::SystemBase* create_system() {
    return new ProcessPnumaticPipeMovement();
}

extern "C" void destroy_system(afterhours::SystemBase* sys) {
    delete sys;
}

} // namespace scripted_systems
```

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Transpiler bugs | Medium | High | Comprehensive test suite, start simple |
| dlopen complexity | Medium | Medium | Abstract behind clean interface, test on all platforms |
| State serialization edge cases | Medium | Medium | Support only primitive types initially |
| Compile time too slow | Low | Medium | Incremental compilation, precompiled headers |

---

## Alternatives Considered

### 1. Lua/Wren/AngelScript

Embed existing scripting language.

**Pros:** Battle-tested, good tooling
**Cons:** Performance overhead, different paradigm, bindings maintenance

### 2. Pure Interpreter

Build custom bytecode interpreter.

**Pros:** Full control, instant reload
**Cons:** Never matches native performance, massive effort

### 3. C++ with #include hot-reload

Use dlopen on C++ directly.

**Pros:** No new language
**Cons:** Templates break, slow compile, complex state management

**Chosen:** Transpile custom language to C++ for native performance with hot-reload via dylib swapping.

