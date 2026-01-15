# Complexity Reduction Report (Phase 1)

This write-up summarizes high-branching and high side‑effect hotspots found in
`src/` during Phase 1. The goal is to make future refactors targeted and
measurable by identifying where control flow and mutation are concentrated.

## High-Branching Hotspots

These functions have dense branching (large switches, nested conditionals, or
multi-path control flow). They are good candidates for decomposition into
smaller, single‑purpose helpers.

- `system_manager::trigger_cb_on_full_progress` (`src/system/afterhours_sixtyfps_systems.cpp`)
  - Large `switch` on `IsTriggerArea::Type` with per‑case logic that spans
    state transitions, networking, save/load, and entity mutations.

```84:359:/Users/gabeochoa/p/pharmasea/src/system/afterhours_sixtyfps_systems.cpp
void trigger_cb_on_full_progress(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;
    // ...
    switch (ita.type) {
        case IsTriggerArea::Store_Reroll: { /* ... */ } break;
        case IsTriggerArea::ModelTest_BackToLobby: { /* ... */ } break;
        case IsTriggerArea::Lobby_ModelTest: { /* ... */ } break;
        case IsTriggerArea::Lobby_LoadSave: { /* ... */ } break;
        case IsTriggerArea::Lobby_PlayGame: { /* ... */ } break;
        case IsTriggerArea::Progression_Option1: /* ... */ break;
        case IsTriggerArea::Progression_Option2: /* ... */ break;
        case IsTriggerArea::Store_BackToPlanning: { /* ... */ } break;
        case IsTriggerArea::LoadSave_BackToLobby: { /* ... */ } break;
        case IsTriggerArea::LoadSave_LoadSlot: { /* ... */ } break;
        case IsTriggerArea::LoadSave_ToggleDeleteMode: { /* ... */ } break;
        case IsTriggerArea::Planning_SaveSlot: { /* ... */ } break;
    }
}
```

- `system_manager::update_dynamic_trigger_area_settings`
  - Multiple nested switches, special‑case handling, and fallback logic.

```362:519:/Users/gabeochoa/p/pharmasea/src/system/afterhours_sixtyfps_systems.cpp
void update_dynamic_trigger_area_settings(Entity& entity, float) {
    // ...
    switch (ita.type) {
        case IsTriggerArea::ModelTest_BackToLobby: { /* ... */ } break;
        case IsTriggerArea::Lobby_ModelTest: { /* ... */ } break;
        case IsTriggerArea::Lobby_PlayGame: { /* ... */ } break;
        case IsTriggerArea::Lobby_LoadSave: { /* ... */ } break;
        case IsTriggerArea::Store_BackToPlanning: { /* ... */ } break;
        case IsTriggerArea::LoadSave_BackToLobby: { /* ... */ } break;
        case IsTriggerArea::LoadSave_ToggleDeleteMode: { /* ... */ } break;
        case IsTriggerArea::Planning_SaveSlot: { /* ... */ } break;
        case IsTriggerArea::Store_Reroll: { /* ... */ } break;
        case IsTriggerArea::LoadSave_LoadSlot: { /* ... */ } break;
        // ...
    }
    // ... second switch
}
```

- `items::process_drink_working` (`src/entity_makers.cpp`)
  - Multiple early exits plus two nested sub‑flows (beer tap vs ingredient
    add) that each mutate shared state.

```1026:1108:/Users/gabeochoa/p/pharmasea/src/entity_makers.cpp
void process_drink_working(Entity& drink, HasWork& hasWork, Entity& player,
                           float dt) {
    // ...
    const auto _process_if_beer_tap = [&]() { /* ... */ };
    auto _process_add_ingredient = [&]() { /* ... */ };
    _process_if_beer_tap();
    _process_add_ingredient();
}
```

- `convert_to_type` + `items::make_item_type` (`src/entity_makers.cpp`)
  - Large switches mapping `EntityType` to constructors, interleaving
    validation and creation logic.

```1594:1770:/Users/gabeochoa/p/pharmasea/src/entity_makers.cpp
bool convert_to_type(const EntityType& entity_type, Entity& entity, vec2 location) {
    switch (entity_type) {
        case EntityType::RemotePlayer: { /* ... */ } break;
        case EntityType::Player: { /* ... */ } break;
        // many cases...
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        // ...
            return false;
    }
    return true;
}
```

- `Map::generate_model_test_map` + `Map::generate_store_map`
  - Deep branching from per‑entity creation and per‑trigger validation.

```178:393:/Users/gabeochoa/p/pharmasea/src/map_generation.cpp
void Map::generate_model_test_map() {
    const auto custom_spawner = [](const ModelTestMapInfo& mtmi, Entity& entity,
                                   vec2 location) {
        switch (mtmi.et) {
            case EntityType::Table: { /* nested switch */ } break;
            case EntityType::FruitBasket: /* ... */ break;
            case EntityType::SingleAlcohol: /* ... */ break;
            // ...
        }
    };
    // ...
}
```

```491:639:/Users/gabeochoa/p/pharmasea/src/map_generation.cpp
void Map::generate_store_map() {
    // ...
    entity.get<IsTriggerArea>()
        .set_validation_fn([](const IsTriggerArea& ita) -> ValidationResult {
            // many branches: balance, cart, required machines, etc.
        });
    // ...
}
```

- `SystemManager::_create_nuxes` (`src/system/system_manager.cpp`)
  - Large, multi‑step tutorial flow with nested branching and entity creation.

```412:850:/Users/gabeochoa/p/pharmasea/src/system/system_manager.cpp
bool _create_nuxes(Entity&) {
    // ... multiple tutorial steps, each with their own branching
    log_info("created nuxes");
    return true;
}
```

- `ProcessConveyerItemsSystem::for_each_with`
  - Multiple decision paths around conveyer direction, matching, and
    hand‑off logic.

```227:370:/Users/gabeochoa/p/pharmasea/src/system/afterhours_inround_systems.cpp
virtual void for_each_with(Entity& entity, Transform& transform,
                           CanHoldItem& canHold,
                           ConveysHeldItem& conveysHeldItem,
                           CanBeTakenFrom& canBeTakenFrom, float dt) override {
    // lots of conditional paths + switch on directions
}
```

- Input handlers in `src/system/input_process_manager.cpp`
  - Long procedures that combine input decoding, movement, item/furniture
    logic, and side effects.

```1161:1255:/Users/gabeochoa/p/pharmasea/src/system/input_process_manager.cpp
void process_input(Entity& entity, const UserInput& input) {
    const auto _proc_single_input_name = [](Entity& entity,
                                            const InputName& input_name,
                                            float input_amount, float frame_dt,
                                            float cam_angle) {
        switch (input_name) { /* ... */ }
        switch (input_name) { /* ... */ }
    };
    // ...
}
```

## High Side‑Effect Hotspots

These functions concentrate state mutation, IO, or networking. They are good
candidates for side‑effect isolation and interface boundaries.

- `startup`, `process_dev_flags`, and `main` (`src/game.cpp`)
  - Global flags, file IO, networking init, and layer wiring.

```109:598:/Users/gabeochoa/p/pharmasea/src/game.cpp
void startup() { /* creates App, Files, Preload, network init, layers */ }
void process_dev_flags(int argc, char* argv[]) { /* many globals */ }
int main(int argc, char* argv[]) { /* mode switches, App::run */ }
```

- `Server::process_map_update` (`src/network/server.cpp`)
  - Save/load, map state replacement, forced syncs, and per‑tick updates.

```341:412:/Users/gabeochoa/p/pharmasea/src/network/server.cpp
void Server::process_map_update(float dt) {
    // CLI save loading, map replacement, seed changes, force_send_map_state
    pharmacy_map->_onUpdate(temp_players, dt);
}
```

- `system_manager::move_player_SERVER_ONLY` (`src/system/system_manager.cpp`)
  - Teleports players and sends packets.

```61:116:/Users/gabeochoa/p/pharmasea/src/system/system_manager.cpp
void move_player_SERVER_ONLY(Entity& entity, game::State location) {
    // pick location, update transform, send network packet
}
```

- `system_manager::spawn_machines_for_newly_unlocked_drink_DONOTCALL`
  - Bulk entity creation and progression unlocks.

```223:333:/Users/gabeochoa/p/pharmasea/src/system/system_manager.cpp
void spawn_machines_for_newly_unlocked_drink_DONOTCALL(
    IsProgressionManager& ipm, Drink option) { /* big switch */ }
```

- `system_manager::store::move_purchased_furniture`
  - Moves entities, updates held items, and mutates bank/cart state.

```921:990:/Users/gabeochoa/p/pharmasea/src/system/system_manager.cpp
void move_purchased_furniture() {
    // move entities, update held items, update bank/cart
}
```

- `system_manager::ProcessSpawnerSystem::for_each_with`
  - Spawns new entities and triggers sound side effects.

```425:461:/Users/gabeochoa/p/pharmasea/src/system/afterhours_inround_systems.cpp
void for_each_with(Entity& entity, Transform& transform, IsSpawner& spawner,
                   float dt) override {
    // validates, spawns new entities, plays sound
}
```

- `system_manager::ProcessHasRopeSystem::for_each_with`
  - Pathfinding + entity creation for rope segments.

```584:645:/Users/gabeochoa/p/pharmasea/src/system/afterhours_inround_systems.cpp
void for_each_with(Entity& entity, CanHoldItem& chi, HasRopeToItem& hrti, float) override {
    // pathfind + create many entities
}
```

## Notes for Phase 2

If we proceed to a plan, we can group refactors into:
- Trigger‑area flows (extract per‑type handlers + shared helpers).
- Entity creation switches (table-driven factory registration).
- Input pipelines (split decoding, state mutation, audio/FX).
- Map generation validation (move validation logic into separate validators).
- High‑impact side effects (wrap IO/network/creation behind explicit services).

