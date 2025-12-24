
#include "client_server_comm.h"

#include "network/server.h"
#include "save_game/save_game.h"
#include "engine/random_engine.h"
#include "entity_helper.h"
#include "components/has_dynamic_model_name.h"
#include "components/has_fishing_game.h"
#include "components/has_subtype.h"
#include "components/is_drink.h"
#include "dataclass/ingredient.h"
#include "recipe_library.h"

namespace server_only {
static void reinit_dynamic_model_names_after_load() {
    // Loaded saves restore ECS data, but many components contain runtime-only
    // callbacks (std::function) that are not serialized. Recreate the dynamic
    // model-name fetchers here so we don't call empty std::function at runtime.
    for (const auto& sp : server_entities_DO_NOT_USE) {
        if (!sp) continue;
        Entity& e = *sp;

        if (e.has<HasDynamicModelName>()) {
            e.removeComponent<HasDynamicModelName>();
        }

        switch (e.entity_type) {
            case EntityType::Cupboard: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Cupboard,
                    HasDynamicModelName::DynamicType::OpenClosed);
            } break;
            case EntityType::Champagne: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Champagne,
                    HasDynamicModelName::DynamicType::Ingredients,
                    [](const Entity& owner, const std::string&) -> std::string {
                        return owner.get<HasFishingGame>().has_score()
                                   ? "champagne_open"
                                   : "champagne";
                    });
            } break;
            case EntityType::Alcohol: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Alcohol, HasDynamicModelName::DynamicType::Subtype,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const HasSubtype& hst = owner.get<HasSubtype>();
                        Ingredient bottle = get_ingredient_from_index(
                            (int) ingredient::AlcoholsInCycle[0] +
                            hst.get_type_index());
                        return util::toLowerCase(
                            magic_enum::enum_name<Ingredient>(bottle));
                    });
            } break;
            case EntityType::Fruit: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Fruit, HasDynamicModelName::DynamicType::Subtype,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const HasSubtype& hst = owner.get<HasSubtype>();
                        Ingredient fruit = ingredient::Fruits[0 + hst.get_type_index()];
                        return util::convertToSnakeCase<Ingredient>(fruit);
                    });
            } break;
            case EntityType::Drink: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Drink,
                    HasDynamicModelName::DynamicType::Ingredients,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const IsDrink& isdrink = owner.get<IsDrink>();
                        constexpr auto drinks = magic_enum::enum_values<Drink>();
                        for (Drink d : drinks) {
                            if (isdrink.matches_drink(d))
                                return get_model_name_for_drink(d);
                        }
                        return util::convertToSnakeCase<EntityType>(EntityType::Drink);
                    });
            } break;
            case EntityType::Pitcher: {
                e.addComponent<HasDynamicModelName>().init(
                    EntityType::Pitcher,
                    HasDynamicModelName::DynamicType::Ingredients,
                    [](const Entity& owner, const std::string&) -> std::string {
                        const IsDrink& isdrink = owner.get<IsDrink>();
                        constexpr auto drinks = magic_enum::enum_values<Drink>();
                        for (Drink d : drinks) {
                            if (isdrink.matches_drink(d))
                                return get_model_name_for_drink(d);
                        }
                        return util::convertToSnakeCase<EntityType>(EntityType::DraftTap);
                    });
            } break;
            case EntityType::FruitJuice: {
                // NOTE: the specific fruit is currently not persisted (it was a
                // captured lambda at spawn-time). Leave this without a dynamic
                // model name for now, and log loudly so we can fix it properly.
                log_error(
                    "Loaded FruitJuice without persisted subtype; dynamic model name can't be restored yet (entity id {})",
                    e.id);
            } break;
            default: {
                // Most entities don't use HasDynamicModelName; nothing to do.
            } break;
        }
    }
}

void play_sound(const vec2& location, strings::sounds::SoundId sound_id) {
    network::Server::play_sound(location, sound_id);
}

void set_show_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->showMinimap = true;
}
void set_hide_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->showMinimap = false;
}

void update_seed(const std::string& seed) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    server->get_map_SERVER_ONLY()->update_seed(seed);
}
std::string get_current_seed() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    return server->get_map_SERVER_ONLY()->seed;
}

bool save_game_to_slot(int slot) {
    if (!is_server()) {
        log_warn("save_game_to_slot called from non-server context");
        return false;
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    if (!server) return false;

    // Capture snapshot from authoritative state.
    server->get_map_SERVER_ONLY()->grab_things();
    Map snapshot = *(server->get_map_SERVER_ONLY());
    snapshot.game_info.was_generated = true;

    bool ok = save_game::SaveGameManager::save_slot(slot, snapshot);
    if (ok) {
        log_info("Saved game to slot {}", slot);
    } else {
        log_warn("Failed to save game to slot {}", slot);
    }
    return ok;
}

bool delete_game_slot(int slot) {
    if (!is_server()) return false;
    bool ok = save_game::SaveGameManager::delete_slot(slot);
    if (!ok) log_warn("Failed to delete save slot {}", slot);
    return ok;
}

bool load_game_from_slot(int slot) {
    if (!is_server()) {
        log_warn("load_game_from_slot called from non-server context");
        return false;
    }
    network::Server* server = GLOBALS.get_ptr<network::Server>("server");
    if (!server) return false;

    save_game::SaveGameFile loaded;
    if (!save_game::SaveGameManager::load_slot(slot, loaded)) {
        log_warn("Failed to load save slot {}", slot);
        return false;
    }

    // Install entities into the authoritative entity list and mark generated
    // so the generator does not wipe the loaded snapshot.
    server_entities_DO_NOT_USE = loaded.map_snapshot.game_info.entities;

    Map& server_map = *(server->get_map_SERVER_ONLY());
    server_map.game_info = loaded.map_snapshot.game_info;
    server_map.showMinimap = loaded.map_snapshot.showMinimap;

    server_map.seed = loaded.map_snapshot.game_info.seed;
    server_map.game_info.was_generated = true;
    RandomEngine::set_seed(server_map.seed);

    reinit_dynamic_model_names_after_load();

    EntityHelper::invalidateCaches();

    // Push an update quickly so clients snap to the new world.
    server->force_send_map_state();

    log_info("Loaded save slot {} (seed='{}')", slot, server_map.seed);
    return true;
}
}  // namespace server_only
