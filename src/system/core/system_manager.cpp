

#include "system_manager.h"

#include "../afterhours/afterhours_systems.h"
#include "../helpers/store_management_helpers.h"
#include "entity.h"

///
#include "../../building_locations.h"
#include "../../components/can_change_settings_interactively.h"
#include "../../components/can_hold_item.h"
#include "../../components/custom_item_position.h"
#include "../../components/has_ai_queue_state.h"
#include "../../components/has_day_night_timer.h"
#include "../../components/has_name.h"
#include "../../components/has_subtype.h"
#include "../../components/indexer.h"
#include "../../components/is_bank.h"
#include "../../components/is_drink.h"
#include "../../components/is_floor_marker.h"
#include "../../components/is_free_in_store.h"
#include "../../components/is_item_container.h"
#include "../../components/is_nux_manager.h"
#include "../../components/is_progression_manager.h"
#include "../../components/is_round_settings_manager.h"
#include "../../components/is_store_spawned.h"
#include "../../components/responds_to_user_input.h"
#include "../../components/simple_colored_box_renderer.h"
#include "../../components/transform.h"
#include "../../dataclass/upgrades.h"
#include "../../engine/util.h"
#include "../../entity_id.h"
#include "raylib.h"
#include "sophie.h"
///
#include "../../engine/pathfinder.h"
#include "../../engine/runtime_globals.h"
#include "../../engine/tracy.h"
#include "../../entity_helper.h"
#include "../../entity_query.h"
#include "../../map.h"
#include "../../network/server.h"
#include "../ai/ai_system.h"
#include "../helpers/ingredient_helper.h"
#include "../helpers/progression.h"
#include "../input/input_process_manager.h"
#include "../rendering/rendering_system.h"
#include "../ui/ui_rendering_system.h"
#include "magic_enum/magic_enum.hpp"

namespace system_manager {

vec3 get_new_held_position_custom(Entity& entity);
vec3 get_new_held_position_default(Entity& entity);

void move_player_SERVER_ONLY(Entity& entity, game::State location) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client context, "
            "this is best case a no-op and worst case a visual desync");
    }

    vec3 position;
    switch (location) {
        case game::Paused:  // fall through
        case game::InMenu:
            return;
            break;
        case game::Lobby: {
            position = LOBBY_BUILDING.to3();
        } break;
        case game::InGame: {
            OptEntity spawn_area =
                EQ().whereFloorMarkerOfType(IsFloorMarker::Planning_SpawnArea)
                    .gen_first();
            if (!spawn_area) {
                position = {0, 0, 0};
            } else {
                // this is a guess based off the current size of the trigger
                // area
                // TODO read the actual size?
                // TODO validate nothing is already there
                vec2 pos = spawn_area.asE().get<Transform>().as2();
                position = {pos.x, 0, pos.y + 3};
            }
        } break;
        case game::ModelTest: {
            position = MODEL_TEST_BUILDING.to3();
        } break;
        case game::LoadSaveRoom: {
            position = LOAD_SAVE_BUILDING.to3();
        } break;
    }

    Transform& transform = entity.get<Transform>();
    transform.update(position);

    // TODO if we have multiple local players then we need to specify which here

    network::Server* server = globals::server();
    if (!server) return;

    int client_id = server->get_client_id_for_entity(entity);
    if (client_id == -1) {
        log_warn("Tried to find a client id for entity but didnt find one");
        return;
    }

    server->send_player_location_packet(client_id, position, transform.facing,
                                        entity.get<HasName>().name());
}

template<typename... TArgs>
void backfill_empty_container(const EntityType& match_type, Entity& entity,
                              vec3 pos, TArgs&&... args) {
    if (entity.is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity.get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    Entity& item =
        EntityHelper::createItem(iic.type(), pos, std::forward<TArgs>(args)...);
    canHold.update(item, entity.id);
}

void fix_container_item_type(Entity& entity) {
    if (entity.is_missing<IsItemContainer>()) return;

    IsItemContainer& iic = entity.get<IsItemContainer>();
    EntityType entity_type = get_entity_type(entity);

    // Map container entity types to the item types they should hold
    switch (entity_type) {
        case EntityType::FruitBasket:
            iic.set_item_type(EntityType::Fruit).set_uses_indexer(true);
            break;
        case EntityType::MopHolder:
            iic.set_item_type(EntityType::Mop).set_max_generations(1);
            break;
        case EntityType::MopBuddyHolder:
            iic.set_item_type(EntityType::MopBuddy);
            break;
        case EntityType::SimpleSyrupHolder:
            iic.set_item_type(EntityType::SimpleSyrup);
            break;
        case EntityType::ChampagneHolder:
            iic.set_item_type(EntityType::Champagne);
            break;
        case EntityType::AlcoholCabinet:
            iic.set_item_type(EntityType::Alcohol).set_uses_indexer(true);
            break;
        case EntityType::Cupboard:
            iic.set_item_type(EntityType::Pitcher);
            break;
        case EntityType::PitcherCupboard:
            iic.set_item_type(EntityType::Pitcher);
            break;
        case EntityType::SodaMachine:
            iic.set_item_type(EntityType::SodaSpout);
            break;
        case EntityType::Blender:
            iic.set_item_type(EntityType::Drink);
            break;
        case EntityType::DraftTap:
            iic.set_item_type(EntityType::Alcohol);
            break;
        case EntityType::Trash:
            iic.set_item_type(EntityType::Trash);
            break;
        default:
            // Unknown container type, leave as Unknown
            break;
    }
}

void process_is_container_and_should_backfill_item(Entity& entity, float) {
    if (entity.is_missing<IsItemContainer>()) return;
    const IsItemContainer& iic = entity.get<IsItemContainer>();

    if (entity.is_missing<CanHoldItem>()) return;
    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    auto pos = entity.get<Transform>().pos();

    if (iic.should_use_indexer() && entity.has<Indexer>()) {
        Indexer& indexer = entity.get<Indexer>();

        // TODO :backfill-correct: This should match whats in container_haswork

        // TODO For now we are okay doing this because the other indexer
        // (alcohol) always unlocks rum first which is index 0. if that changes
        // we gotta update this
        //  --> should we just add an assert here so we catch it quickly?
        if (check_type(entity, EntityType::FruitBasket)) {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipp =
                sophie.get<IsProgressionManager>();

            indexer.increment_until_valid([&](int index) {
                return !ipp.is_ingredient_locked(ingredient::Fruits[index]);
            });
        }

        backfill_empty_container(iic.type(), entity, pos, indexer.value());
        entity.get<Indexer>().mark_change_completed();
        return;
    }

    backfill_empty_container(iic.type(), entity, pos);
}

void update_visuals_for_settings_changer(Entity& entity, float) {
    if (entity.is_missing<CanChangeSettingsInteractively>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    auto get_name = [](CanChangeSettingsInteractively::Style style,
                       bool bool_value) {
        auto bool_str = bool_value ? "Enabled" : "Disabled";

        switch (style) {
            case CanChangeSettingsInteractively::ToggleIsTutorial:
                return fmt::format("Tutorial: {}", bool_str);
                break;
            case CanChangeSettingsInteractively::Unknown:
                break;
        }
        log_warn(
            "Tried to get_name for Interactive Setting Style {} but it was "
            "not "
            "handled",
            magic_enum::enum_name<CanChangeSettingsInteractively::Style>(
                style));
        return std::string("");
    };

    auto update_color_for_bool = [](Entity& isc, bool value) {
        // TODO eventually read from theme... (imports)
        // auto color = value ? UI_THEME.from_usage(theme::Usage::Primary)
        // : UI_THEME.from_usage(theme::Usage::Error);

        auto color = value ?  //
                         ::ui::color::green_apple
                           : ::ui::color::red;

        isc.get<SimpleColoredBoxRenderer>()  //
            .update_face(color)
            .update_base(color);
    };

    auto style = entity.get<CanChangeSettingsInteractively>().style;

    switch (style) {
        case CanChangeSettingsInteractively::ToggleIsTutorial: {
            entity.get<HasName>().update(
                get_name(style, irsm.interactive_settings.is_tutorial_active));
            update_color_for_bool(entity,
                                  irsm.interactive_settings.is_tutorial_active);
        } break;
        case CanChangeSettingsInteractively::Unknown:
            break;
    }
}

}  // namespace system_manager

void SystemManager::update_all_entities(const Entities& players, float dt) {
    // TODO speed?
    Entities entities;
    Entities ents = EntityHelper::get_entities();

    entities.reserve(players.size() + ents.size());

    // Filter out null and cleanup entities when building the list
    for (const auto& ent : ents) {
        if (ent && !ent->cleanup) {
            entities.push_back(ent);
        }
    }
    for (const auto& player : players) {
        if (player && !player->cleanup) {
            entities.push_back(player);
        }
    }

    oldAll = entities;

    // should we not do any updates for client?
    // any changes will get overwritten by server every frame
    // but maybe itll matter
    //
    // TODO maybe this should only NOT run for the host? since the latency
    // is decent
    //
    // This check might cause lots of issues when high latency
    if (!is_server()) return;

    timePassed += dt;

    // NOTE: Old system functions are now handled by afterhours systems
    // The systems have should_run() methods that conditionally enable them
    // based on game state, matching the original conditional logic.
    //
    // Note: SixtyFpsUpdateSystem runs every frame (not just at 60fps) for
    // better responsiveness, especially for trigger areas. The timePassed
    // accumulator is kept for potential future use but is no longer needed for
    // system timing. We still reset it to maintain the original timing pattern.
    if (timePassed >= 0.016f) {
        timePassed = 0;
    }

    // actual update
    {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());

        PathRequestManager::process_responses(entities);
        // Run afterhours systems
        // Note: systems.tick() expects a non-const Entities&
        // We use oldAll which contains the same entities but is mutable
        systems.tick(oldAll, dt);
    }
}

void SystemManager::update_remote_players(const Entities& players, float) {
    remote_players = players;
}

void SystemManager::update_local_players(const Entities& players, float dt) {
    local_players = players;
    for (const auto& entity : players) {
        system_manager::input_process_manager::collect_user_input(*entity, dt);
    }
}

void SystemManager::process_inputs(const Entities& entities,
                                   const UserInputs& inputs) {
    for (const auto& entity : entities) {
        if (entity->is_missing<RespondsToUserInput>()) continue;
        for (auto input : inputs) {
            system_manager::input_process_manager::process_input(*entity,
                                                                 input);
        }
    }
}

void SystemManager::render_entities(const Entities& entities, float dt) const {
    // NOTE: Rendering is now handled by RenderEntitiesSystem
    // The system's once() method handles on_frame_start() and
    // render_walkable_spots() The system's for_each_with() method handles
    // rendering each entity We use systems.render() which expects const
    // Entities&
    systems.render(entities, dt);
}

void SystemManager::render_ui(const Entities& entities, float dt) const {
    // const auto debug_mode_on =
    // GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    system_manager::ui::render_normal(entities, dt);
}

bool SystemManager::is_bar_open() const {
    for (const auto& entity : oldAll) {
        if (entity->is_missing<HasDayNightTimer>()) continue;
        return entity->get<HasDayNightTimer>().is_bar_open();
    }
    return false;
}
bool SystemManager::is_bar_closed() const { return !is_bar_open(); }
