# Replace convert_to_type Switch with Factory Map

**Category:** Entity Creation Simplification
**Impact:** ~100 lines saved
**Risk:** Low
**Complexity:** Low

## Current State

`convert_to_type()` is a 176-line switch statement dispatching to make_* functions.

## Refactoring

Function pointer map:

```cpp
using EntityMaker = void(*)(Entity&, vec2);

static const std::unordered_map<EntityType, EntityMaker> ENTITY_MAKERS = {
    {EntityType::Player, make_player},
    {EntityType::Customer, make_customer},
    {EntityType::Table, make_table},
    {EntityType::Register, make_register},
    // ... all types
};

void convert_to_type(Entity& entity, EntityType type, vec2 pos) {
    auto it = ENTITY_MAKERS.find(type);
    if (it != ENTITY_MAKERS.end()) {
        it->second(entity, pos);
    } else {
        log_warn("Unknown entity type: {}", magic_enum::enum_name(type));
    }
}
```

## Impact

Replaces 176 lines of switch with ~80 lines of map initialization, easier to maintain.

## Benefits

- Adding new entity types is one line in the map
- No chance of missing a case in switch
- Easy to iterate over all registered types if needed
- Can be extended to include metadata per type

## Files Affected

- `src/entity_makers.cpp`
