# Data-Driven Entity Makers

**Category:** Entity Creation Simplification
**Impact:** ~400-500 lines saved
**Risk:** Medium
**Complexity:** Medium

## Current State

`entity_makers.cpp` is 1,775 lines with repetitive patterns:

1. **Item containers** (8 functions, ~120 lines): `make_single_alcohol()`, `make_medicine_cabinet()`, `make_fruit_basket()`, etc. all follow:
   ```cpp
   furniture::make_itemcontainer(container, {EntityType::XXX}, pos, EntityType::YYY);
   container.addComponent<Indexer>(size);
   container.get<IsItemContainer>().set_uses_indexer(true);
   ```

2. **Ingredient-adding furniture** (3 functions, ~80 lines): `make_ice_machine()`, `make_draft()`, `make_simple_syrup()` all add ingredients to drinks

3. **Work lambda boilerplate** (8+ functions): Identical `HasWork().init()` patterns

## Basic Refactoring

Configuration structs + factory functions:

```cpp
struct ItemContainerSpec {
    EntityType container_type;
    EntityType held_type;
    int indexer_size = 0;
    bool uses_indexer = false;
    bool is_table_when_empty = false;
};

void make_item_container(Entity& entity, vec2 pos, const ItemContainerSpec& spec) {
    furniture::make_itemcontainer(entity, {spec.container_type}, pos, spec.held_type);
    if (spec.uses_indexer) {
        entity.addComponent<Indexer>(spec.indexer_size);
        entity.get<IsItemContainer>().set_uses_indexer(true);
    }
    if (spec.is_table_when_empty) {
        entity.get<IsItemContainer>().set_is_table_when_empty(true);
    }
}

// Usage:
constexpr ItemContainerSpec MEDICINE_CABINET = {
    .container_type = EntityType::MedicineCabinet,
    .held_type = EntityType::Medicine,
    .indexer_size = 4,
    .uses_indexer = true
};

void make_medicine_cabinet(Entity& e, vec2 pos) {
    make_item_container(e, pos, MEDICINE_CABINET);
}
```

Similarly for ingredient-adding furniture:

```cpp
struct IngredientFurnitureSpec {
    EntityType furniture_type;
    Ingredient ingredient;
    float work_speed = 1.0f;
    std::string work_sound;
};

void make_ingredient_furniture(Entity& entity, vec2 pos, const IngredientFurnitureSpec& spec);
```

## Deeper Refactoring Discussion

Now that we understand the patterns, there are several directions to explore:

### Observed Entity Categories

1. **Item Containers** - hold items, optionally indexed, optionally act as table when empty
2. **Ingredient Adders** - furniture that adds ingredients to drinks (ice machine, draft, syrup)
3. **Item Holders** - simple holders for single item types (mop holder, champagne holder)
4. **Work Stations** - furniture with work progress (table, toilet, fruit basket)
5. **Spawners** - create entities over time
6. **Conveyors** - move items between locations
7. **Characters** - players and AI (customers, mop buddy)

### Approach A: Component Bundles / Archetypes

Define common component combinations as reusable bundles:

```cpp
// Predefined bundles
void apply_furniture_base(Entity& e, EntityType type, vec2 pos);
void apply_item_container(Entity& e, EntityType held_type, int capacity);
void apply_work_station(Entity& e, float work_speed, WorkCallback on_complete);
void apply_ingredient_adder(Entity& e, Ingredient ing, float speed);

// Usage - compose from bundles
void make_ice_machine(Entity& e, vec2 pos) {
    apply_furniture_base(e, EntityType::IceMachine, pos);
    apply_item_container(e, EntityType::IceBag, 4);
    apply_ingredient_adder(e, Ingredient::Ice, 1.5f);
}
```

### Approach B: Entity Blueprints / Prefabs

Define entities entirely in data, load at runtime:

```cpp
// Could be JSON or constexpr structs
EntityBlueprint ICE_MACHINE = {
    .type = EntityType::IceMachine,
    .components = {
        FurnitureBase{},
        ItemContainer{.held_type = EntityType::IceBag, .capacity = 4},
        IngredientAdder{.ingredient = Ingredient::Ice, .speed = 1.5f},
        HasWork{.speed = 1.5f}
    }
};

Entity& e = create_from_blueprint(ICE_MACHINE, pos);
```

### Approach C: Builder Pattern

Fluent API for entity construction:

```cpp
void make_ice_machine(Entity& e, vec2 pos) {
    EntityBuilder(e, pos)
        .as_furniture(EntityType::IceMachine)
        .with_item_container(EntityType::IceBag, 4)
        .adds_ingredient(Ingredient::Ice)
        .with_work(1.5f, sounds::ice_machine)
        .build();
}
```

### Approach D: Trait-Based Composition

Define traits that entities can have, auto-derive behavior:

```cpp
// Traits are marker types that imply component sets
using IceMachine = EntityWith<
    FurnitureTrait,
    ItemContainerTrait<EntityType::IceBag, 4>,
    IngredientAdderTrait<Ingredient::Ice>,
    WorkStationTrait<1.5f>
>;
```

## Afterhours ECS Enhancements to Explore

Could the afterhours framework provide helpers for this?

### 1. Component Bundles

```cpp
// In afterhours - define a bundle of components that go together
template<typename... Components>
struct ComponentBundle {
    std::tuple<Components...> components;

    void apply_to(Entity& e) {
        std::apply([&e](auto&... c) {
            (e.addComponent<std::decay_t<decltype(c)>>(c), ...);
        }, components);
    }
};

// Usage
auto furniture_bundle = make_bundle(
    Transform{pos},
    ModelRenderer{type},
    IsSolid{}
);
furniture_bundle.apply_to(entity);
```

### 2. Entity Archetypes

```cpp
// Register archetype once
auto ice_machine_archetype = ecs.register_archetype<
    Transform, ModelRenderer, IsSolid, IsItemContainer, HasWork
>("IceMachine");

// Create entities from archetype (pre-allocates component storage)
Entity& e = ecs.create_from_archetype(ice_machine_archetype);
```

### 3. Deferred Component Initialization

```cpp
// Add components with deferred init - useful for circular refs
entity.addComponentDeferred<HasWork>([](HasWork& hw, Entity& self) {
    hw.init(self, 1.5f, on_complete_callback);
});
```

### 4. Component Inheritance/Composition in ECS

```cpp
// Define component that "includes" other components
struct IngredientAdder : ComposedComponent<HasWork, CanHoldItem> {
    Ingredient ingredient;
    // HasWork and CanHoldItem are auto-added when this is added
};
```

### 5. Entity Templates

```cpp
// Define template at compile time
constexpr auto ICE_MACHINE_TEMPLATE = entity_template<
    Transform, ModelRenderer, IsSolid, IsItemContainer, HasWork
>{
    .on_create = [](Entity& e, vec2 pos) {
        e.get<Transform>().update(pos);
        e.get<IsItemContainer>().set_held_type(EntityType::IceBag);
        // etc.
    }
};
```

## Questions to Investigate

1. **What's the right level of abstraction?** Specs/configs vs bundles vs blueprints vs builders?

2. **Compile-time vs runtime?** Constexpr specs are fast but inflexible. JSON blueprints are flexible but slower to load.

3. **How much should afterhours know?** Generic ECS helpers vs Pharmasea-specific factories?

4. **Work callbacks** - The `HasWork().init()` lambdas are the trickiest part. How to make those data-driven without losing type safety?

5. **Serialization** - If entities are defined in specs, does that affect save/load or network sync?

## Recommendation

Start with **Approach A (Component Bundles)** as it's the least invasive:
- Extract common component setups into `apply_*` functions
- Keep individual `make_*` functions but have them compose from bundles
- Doesn't require afterhours changes initially
- Can evolve toward blueprints later if needed

Consider adding **afterhours ComponentBundle** support as a follow-up to make this pattern first-class.

## Files Affected

- `src/entity_makers.cpp` (1,775 lines)
- `src/entity_makers.h`
