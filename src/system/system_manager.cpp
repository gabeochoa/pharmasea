

#include "system_manager.h"

#include "afterhours_systems.h"
#include "store_management_helpers.h"

///
#include "../building_locations.h"
#include "../components/adds_ingredient.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/base_component.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_held.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_be_pushed.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_handtruck.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_fishing_game.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_progression.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_bank.h"
#include "../components/is_drink.h"
#include "../components/is_free_in_store.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_nux_manager.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_rotatable.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_squirter.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_day_night.h"
#include "../components/responds_to_user_input.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../dataclass/upgrades.h"
#include "../engine/util.h"
#include "../entity.h"
#include "raylib.h"
#include "sophie.h"
///
#include "../engine/pathfinder.h"
#include "../engine/tracy.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../map.h"
#include "../network/server.h"
#include "ai_system.h"
#include "ingredient_helper.h"
#include "input_process_manager.h"
#include "magic_enum/magic_enum.hpp"
#include "progression.h"
#include "rendering_system.h"
#include "ui_rendering_system.h"

namespace system_manager {

namespace store {
void cart_management(Entity&, float);
void move_purchased_furniture();
}  // namespace store

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
            OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
                IsFloorMarker::Planning_SpawnArea);
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
    }

    Transform& transform = entity.get<Transform>();
    transform.update(position);

    // TODO if we have multiple local players then we need to specify which here

    network::Server* server = GLOBALS.get_ptr<network::Server>("server");

    int client_id = server->get_client_id_for_entity(entity);
    if (client_id == -1) {
        log_warn("Tried to find a client id for entity but didnt find one");
        return;
    }

    server->send_player_location_packet(client_id, position, transform.facing,
                                        entity.get<HasName>().name());
}

void transform_snapper(Entity& entity, float) {
    if (entity.is_missing<Transform>()) return;
    Transform& transform = entity.get<Transform>();
    transform.update(entity.has<IsSnappable>() ? transform.snap_position()
                                               : transform.raw());
}

void clear_all_floor_markers(Entity& entity, float) {
    if (!check_type(entity, EntityType::FloorMarker)) return;
    if (entity.is_missing<IsFloorMarker>()) return;
    IsFloorMarker& ifm = entity.get<IsFloorMarker>();
    ifm.clear();
}

void mark_item_in_floor_area(Entity& entity, float) {
    if (!check_type(entity, EntityType::FloorMarker)) return;
    if (entity.is_missing<IsFloorMarker>()) return;
    IsFloorMarker& ifm = entity.get<IsFloorMarker>();

    std::vector<int> ids =
        EQ(SystemManager::get().oldAll)
            .whereNotID(entity.id)  // not us
            .whereNotType(EntityType::Player)
            .whereNotType(EntityType::RemotePlayer)
            .whereNotType(EntityType::SodaSpout)
            .whereHasComponent<IsSolid>()
            .whereCollides(
                entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))
            // we want to include store items since it has many floor areas
            .include_store_entities()
            .gen_ids();

    ifm.mark_all(std::move(ids));
}

void process_floor_markers(Entity& entity, float dt) {
    clear_all_floor_markers(entity, dt);
    mark_item_in_floor_area(entity, dt);
}

void update_held_hand_truck_position(Entity& entity, float) {
    if (entity.is_missing_any<Transform, CanHoldHandTruck>()) return;

    const Transform& transform = entity.get<Transform>();
    const CanHoldHandTruck& can_hold_hand_truck =
        entity.get<CanHoldHandTruck>();

    if (can_hold_hand_truck.empty()) return;

    auto new_pos = transform.pos();

    OptEntity hand_truck =
        EntityHelper::getEntityForID(can_hold_hand_truck.hand_truck_id());
    hand_truck->get<Transform>().update(new_pos);
}

void update_held_furniture_position(Entity& entity, float) {
    if (entity.is_missing_any<Transform, CanHoldFurniture>()) return;

    const Transform& transform = entity.get<Transform>();
    const CanHoldFurniture& can_hold_furniture = entity.get<CanHoldFurniture>();

    if (can_hold_furniture.empty()) return;

    auto new_pos = transform.pos();
    if (transform.face_direction() & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }

    OptEntity furniture =
        EntityHelper::getEntityForID(can_hold_furniture.furniture_id());
    furniture->get<Transform>().update(new_pos);
}

vec3 get_new_held_position_custom(Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 new_pos = transform.pos();

    const CustomHeldItemPosition& custom_item_position =
        entity.get<CustomHeldItemPosition>();

    switch (custom_item_position.positioner) {
        case CustomHeldItemPosition::Positioner::Table:
            if (entity.has<ModelRenderer>()) {
                new_pos.y += TILESIZE / 2.f;
            }
            new_pos.y += 0.f;
            break;
        case CustomHeldItemPosition::Positioner::ItemHoldingItem:
            new_pos.x += 0;
            new_pos.y += 0;
            break;
        case CustomHeldItemPosition::Positioner::Blender:
            new_pos.x += 0;
            new_pos.y += TILESIZE * 2.f;
            break;
        case CustomHeldItemPosition::Positioner::Conveyer: {
            if (entity.is_missing<ConveysHeldItem>()) {
                log_warn("A conveyer positioned item needs ConveysHeldItem");
                break;
            }
            const ConveysHeldItem& conveysHeldItem =
                entity.get<ConveysHeldItem>();
            if (transform.face_direction() &
                Transform::FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE * conveysHeldItem.relative_item_pos;
            }
            if (transform.face_direction() &
                Transform::FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE * conveysHeldItem.relative_item_pos;
            }
            new_pos.y += TILESIZE / 4;
        } break;
        case CustomHeldItemPosition::Positioner::PnumaticPipe: {
            if (entity.is_missing<IsPnumaticPipe>()) {
                log_warn("pipe positioned item needs ispnumaticpipe");
                break;
            }
            if (entity.is_missing<ConveysHeldItem>()) {
                log_warn("pipe positioned item needs ConveysHeldItem");
                break;
            }
            const ConveysHeldItem& conveysHeldItem =
                entity.get<ConveysHeldItem>();
            int mult = entity.get<IsPnumaticPipe>().recieving ? 1 : -1;
            new_pos.y += mult * TILESIZE * conveysHeldItem.relative_item_pos;
        } break;
    }
    return new_pos;
}

vec3 get_new_held_position_default(Entity& entity) {
    const Transform& transform = entity.get<Transform>();
    vec3 new_pos = transform.pos();
    // Default
    if (transform.face_direction() & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    return new_pos;
}

// TODO should held item position be a physical movement or just visual?
// does it matter if the reach/pickup is working as expected
void update_held_item_position(Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& can_hold_item = entity.get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    vec3 new_pos = entity.has<CustomHeldItemPosition>()
                       ? get_new_held_position_custom(entity)
                       : get_new_held_position_default(entity);

    can_hold_item.item().get<Transform>().update(new_pos);
}

void reset_highlighted(Entity& entity, float) {
    if (entity.is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity.get<CanBeHighlighted>();
    cbh.update(entity, false);
}

void highlight_facing_furniture(Entity& entity, float) {
    if (entity.is_missing<CanHighlightOthers>()) return;
    const Transform& transform = entity.get<Transform>();
    const CanHighlightOthers& cho = entity.get<CanHighlightOthers>();

    OptEntity match = EntityQuery()
                          .whereInRange(transform.as2(), cho.reach())
                          .whereHasComponent<CanBeHighlighted>()
                          .include_store_entities()
                          .orderByDist(transform.as2())
                          .gen_first();
    if (!match) return;

    match->get<CanBeHighlighted>().update(entity, true);
}

// TODO We need like a temporary storage for this
// TODO ^ we arent even pushing customers anymore
void move_entity_based_on_push_force(Entity& entity, float, vec3& new_pos_x,
                                     vec3& new_pos_z) {
    CanBePushed& cbp = entity.get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force().x;
    cbp.update_x(0.0f);

    new_pos_z.z += cbp.pushed_force().z;
    cbp.update_z(0.0f);
}

void process_grabber_items(Entity& entity, float) {
    const Transform& transform = entity.get<Transform>();

    if (entity.is_missing<CanHoldItem>()) return;
    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    // we are already holding something so
    if (canHold.is_holding_item()) return;

    // Should only run this for conveyers
    if (entity.is_missing<ConveysHeldItem>()) return;
    // Should only run for grabbers
    if (entity.is_missing<CanGrabFromOtherFurniture>()) return;

    ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

    auto behind =
        transform.offsetFaceDirection(transform.face_direction(), 180);
    auto match = EntityHelper::getMatchingEntityInFront(
        transform.as2(), 1.f, behind, [&entity](const Entity& furn) {
            // cant be us
            if (entity.id == furn.id) return false;
            // needs to be able to hold something
            if (furn.is_missing<CanHoldItem>()) return false;
            const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
            // doesnt have anything
            if (furnCHI.empty()) return false;

            // Can we hold the item it has?
            bool can_hold = entity.get<CanHoldItem>().can_hold(
                (furnCHI.const_item()), RespectFilter::All);

            // we cant
            if (!can_hold) return false;

            // We only check CanBe when it exists because everyone else can
            // always be taken from with a grabber
            if (furn.is_missing<CanBeTakenFrom>()) return true;
            return furn.get<CanBeTakenFrom>().can_take_from();
        });

    // No furniture behind us
    if (!match) return;

    // Grab from the furniture match
    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity.get<CanHoldItem>();

    ourCHI.update(EntityHelper::getEntityAsSharedPtr(matchCHI.item()),
                  entity.id);
    matchCHI.update(nullptr, -1);

    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
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
    // ^ cannot be const because converting to SharedPtr v

    // TODO do we need shared pointer here? (vs just id?)
    canHold.update(EntityHelper::getEntityAsSharedPtr(item), entity.id);
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

void process_is_container_and_should_update_item(Entity& entity, float) {
    if (entity.is_missing<IsItemContainer>()) return;
    const IsItemContainer& iic = entity.get<IsItemContainer>();

    if (!iic.should_use_indexer()) return;
    if (entity.is_missing<Indexer>()) return;
    Indexer& indexer = entity.get<Indexer>();
    // user didnt change the index so we are good to wait
    if (indexer.value_same_as_last_render()) return;

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    // Delete the currently held item
    if (canHold.is_holding_item()) {
        canHold.item().cleanup = true;
        canHold.update(nullptr, -1);
    }

    auto pos = entity.get<Transform>().pos();
    backfill_empty_container(iic.type(), entity, pos, indexer.value());
    indexer.mark_change_completed();
}

void process_is_indexed_container_holding_incorrect_item(Entity& entity,
                                                         float) {
    // This function is for when you have an indexed container and you put the
    // item back in but the index had changed.
    //
    // in this case we need to clear the item because otherwise they will both
    // live there which will cause overlap and grab issues.

    if (entity.is_missing<Indexer>()) return;
    const Indexer& indexer = entity.get<Indexer>();

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    if (canHold.empty()) return;

    int current_value = indexer.value();
    int item_value = canHold.item().get<HasSubtype>().get_type_index();

    if (current_value != item_value) {
        canHold.item().cleanup = true;
        canHold.update(nullptr, -1);
    }
}

void handle_autodrop_furniture_when_exiting_planning(Entity& entity) {
    if (entity.is_missing<CanHoldFurniture>()) return;
    const CanHoldFurniture& ourCHF = entity.get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // This code never runs at the moment, because we require
    // that everything is dropped before we continue,
    // this exists just to double check in case you can time it
    // correclty to grab at the wrong time

    // in a world where we need to drop for them live, we would need to find a
    // spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

void delete_customers_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return;
    if (!check_type(entity, EntityType::Customer)) return;

    const Transform& transform = entity.get<Transform>();
    if (vec::distance(transform.as2(), {GATHER_SPOT, GATHER_SPOT}) > 2.f)
        return;
    entity.cleanup = true;
}

void refetch_dynamic_model_names(Entity& entity, float) {
    if (entity.is_missing<HasDynamicModelName>()) return;
    if (entity.is_missing<ModelRenderer>()) return;

    const HasDynamicModelName& hDMN = entity.get<HasDynamicModelName>();
    ModelRenderer& renderer = entity.get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch(entity));
}

void count_all_possible_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    size_t count = EQ(SystemManager::get().oldAll)
                       .whereType(EntityType::Player)
                       .gen_count();

    entity.get<IsTriggerArea>().update_all_entrants(static_cast<int>(count));
}

void count_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    size_t count = EQ(SystemManager::get().oldAll)
                       .whereType(EntityType::Player)
                       .whereCollides(entity.get<Transform>().expanded_bounds(
                           {0, TILESIZE, 0}))
                       .gen_count();

    entity.get<IsTriggerArea>().update_entrants(static_cast<int>(count));
}

void count_in_building_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    std::optional<Building> building = entity.get<IsTriggerArea>().building;
    if (!building) return;

    size_t count =
        EQ(SystemManager::get().oldAll)
            .whereType(EntityType::Player)
            .whereInside(building.value().min(), building.value().max())
            .gen_count();

    entity.get<IsTriggerArea>().update_entrants_in_building(
        static_cast<int>(count));
}
void update_trigger_area_percent(Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

    ita.should_wave()  //
        ? ita.increase_progress(dt)
        : ita.decrease_progress(dt);

    // If no one is currently standing on it or whatever
    // then decrease the cooldown
    if (!ita.should_progress()) ita.decrease_cooldown(dt);
}

void spawn_machines_for_newly_unlocked_drink_DONOTCALL(
    IsProgressionManager& ipm, Drink option) {
    // today we dont have a way to statically know which machines
    // provide which ingredients because they are dynamic
    IngredientBitSet possibleNewIGs = get_req_ingredients_for_drink(option);

    // Because prereqs are handled, we dont need do check for them and can
    // assume that those bits will handle checking for it

    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        // Note we spawn free items in the purchase area so its more obvious
        // that they are free
        IsFloorMarker::Type::Planning_SpawnArea);

    if (!spawn_area) {
        // need to guarantee this exists long before we get here
        log_error("Could not find spawn area entity");
    }

    bitset_utils::for_each_enabled_bit(possibleNewIGs, [&ipm, spawn_area](
                                                           size_t index) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(index);

        const auto make_free_machine = []() -> Entity& {
            auto& entity = EntityHelper::createEntity();
            entity.addComponent<IsFreeInStore>();
            return entity;
        };

        // Already has the machine so we good
        if (IngredientHelper::has_machines_required_for_ingredient(ig))
            return bitset_utils::ForEachFlow::Continue;

        switch (ig) {
            case Soda: {
                // Nothing is needed to do for this since
                // the soda machine is required to play the game
            } break;
            case Beer: {
                auto et = EntityType::DraftTap;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Champagne: {
                auto et = EntityType::ChampagneHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Rum:
            case Tequila:
            case Vodka:
            case Whiskey:
            case Gin:
            case TripleSec:
            case Bitters: {
                int alc_index =
                    index_of<Ingredient, ingredient::AlcoholsInCycle.size()>(
                        ingredient::AlcoholsInCycle, ig);
                auto& entity = make_free_machine();
                furniture::make_single_alcohol(
                    entity, spawn_area->get<Transform>().as2(), alc_index);
            } break;
            case Pineapple:
            case Orange:
            case Coconut:
            case Cranberries:
            case Lime:
            case Lemon: {
                auto et = EntityType::FruitBasket;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case PinaJuice:
            case OrangeJuice:
            case CoconutCream:
            case CranberryJuice:
            case LimeJuice:
            case LemonJuice: {
                auto et = EntityType::Blender;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());

                ipm.unlock_entity(et);
            } break;
            case SimpleSyrup: {
                auto et = EntityType::SimpleSyrupHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case IceCubes:
            case IceCrushed: {
                auto et = EntityType::IceMachine;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Salt:
            case MintLeaf:
                // TODO implement for these once thye have spawners
                break;
            case Invalid:
                break;
        }
        return bitset_utils::ForEachFlow::NormalFlow;
    });
}

void generate_machines_for_new_upgrades() {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_SpawnArea);

    for (EntityType et : irsm.config.store_to_spawn) {
        auto& entity = EntityHelper::createEntity();
        bool success =
            convert_to_type(et, entity, purchase_area->get<Transform>().as2());
        if (!success) {
            entity.cleanup = true;
            log_error(
                "Store spawn of newly unlocked item failed to "
                "generate");
        }
    }
    irsm.config.store_to_spawn.clear();
}

void trigger_cb_on_full_progress(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;

    ita.reset_cooldown();

    const auto _choose_option = [](int option_chosen) {
        // We want to lock the doors until the next day
        // so you can get two upgrades at a time
        for (RefEntity door : EntityQuery()
                                  .whereType(EntityType::Door)
                                  .whereInside(PROGRESSION_BUILDING.min(),
                                               PROGRESSION_BUILDING.max())
                                  .gen()) {
            door.get().addComponentIfMissing<IsSolid>();
        }

        GameState::get().transition_to_game();

        for (RefEntity player : EQ(SystemManager::get().oldAll)
                                    .whereType(EntityType::Player)
                                    .whereInside(PROGRESSION_BUILDING.min(),
                                                 PROGRESSION_BUILDING.max())
                                    .gen()) {
            move_player_SERVER_ONLY(player, game::State::InGame);
        }

        {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            IsProgressionManager& ipm = sophie.get<IsProgressionManager>();
            IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
            // choose given option

            switch (ipm.upgrade_type()) {
                case UpgradeType::None: {
                } break;
                case UpgradeType::Upgrade: {
                    const UpgradeClass& option =
                        (option_chosen == 0 ? ipm.upgradeOption1
                                            : ipm.upgradeOption2);
                    auto optionImpl = make_upgrade(option);
                    optionImpl->onUnlock(irsm.config, ipm);

                    // TODO why does this not show up in the pause menu?
                    irsm.selected_upgrades.push_back(optionImpl);
                    irsm.config.mark_upgrade_unlocked(option);

                    // They will be spawned in upgrade_system at Unlock time

                    generate_machines_for_new_upgrades();

                    break;
                }
                case UpgradeType::Drink: {
                    Drink option = option_chosen == 0 ? ipm.drinkOption1
                                                      : ipm.drinkOption2;

                    // Mark the drink unlocked
                    ipm.unlock_drink(option);
                    // Unlock any igredients it needs
                    auto possibleNewIGs = get_req_ingredients_for_drink(option);
                    bitset_utils::for_each_enabled_bit(
                        possibleNewIGs, [&ipm](size_t index) {
                            Ingredient ig =
                                magic_enum::enum_value<Ingredient>(index);
                            ipm.unlock_ingredient(ig);
                            return bitset_utils::ForEachFlow::NormalFlow;
                        });

                    spawn_machines_for_newly_unlocked_drink_DONOTCALL(ipm,
                                                                      option);
                } break;
            }

            // reset options so it collections new ones next upgrade round
            ipm.next_round();

            //
            system_manager::progression::update_upgrade_variables();
        }
    };

    switch (ita.type) {
        case IsTriggerArea::Store_Reroll: {
            system_manager::store::cleanup_old_store_options();
            system_manager::store::generate_store_options();
            {
                OptEntity sophie =
                    EntityQuery().whereType(EntityType::Sophie).gen_first();
                IsBank& bank = sophie->get<IsBank>();

                IsRoundSettingsManager& irsm =
                    sophie->get<IsRoundSettingsManager>();
                int reroll_price = irsm.get<int>(ConfigKey::StoreRerollPrice);
                bank.withdraw(reroll_price);
                irsm.config.permanently_modify(ConfigKey::StoreRerollPrice,
                                               Operation::Add, 25);
            }

        } break;
        case IsTriggerArea::Unset:
            log_warn("Completed trigger area wait time but was Unset type");
            break;
        case IsTriggerArea::ModelTest_BackToLobby: {
            GameState::get().transition_to_lobby();
            {
                // TODO add a tagging system so we can delete certain sets
                // of ents ^ okay so i tried adding a CreationOptions
                // component this worked great for any furniture, but all
                // the items stayed floating
                //
                // I think it could work but you would need to have items
                // inherit the creation options of the parent? which might
                // cause issues for permanant, when this is a simple brute
                // force solution for now

                // NOTE: pre-building code, we used to use the 100,0 positio
                // as the origin and not the top left corner.
                // this assumption changes the box we delete items in
                // new: 100, 0 => 150, 50
                // old: 70, -30 => 130, 30

                const auto ents = EntityHelper::getAllInRange(
                    MODEL_TEST_BUILDING.min(), MODEL_TEST_BUILDING.max());

                for (Entity& to_delete : ents) {
                    // TODO add a way to skip the permananent ones
                    if (to_delete.has<IsTriggerArea>()) continue;
                    to_delete.cleanup = true;
                }
            }
            SystemManager::get().for_each_old([](Entity& e) {
                if (!check_type(e, EntityType::Player)) return;
                move_player_SERVER_ONLY(e, game::State::Lobby);
            });
        } break;
        case IsTriggerArea::Lobby_ModelTest: {
            GameState::get().transition_to_model_test();
            if (is_server()) {
                network::Server* server =
                    GLOBALS.get_ptr<network::Server>("server");
                server->get_map_SERVER_ONLY()
                    ->game_info.generate_model_test_map();
            }

            SystemManager::get().for_each_old([](Entity& e) {
                if (!check_type(e, EntityType::Player)) return;
                move_player_SERVER_ONLY(e, game::State::ModelTest);
            });
        } break;

        case IsTriggerArea::Lobby_PlayGame: {
            GameState::get().transition_to_game();
            SystemManager::get().for_each_old([](Entity& e) {
                if (!check_type(e, EntityType::Player)) return;
                move_player_SERVER_ONLY(e, game::State::InGame);
            });
        } break;
        case IsTriggerArea::Progression_Option1:
            _choose_option(0);
            break;
        case IsTriggerArea::Progression_Option2:
            _choose_option(1);
            break;
            // TODO rename this and change text to "order for delivery"
            // or something  (maybe delivery is an upgrade?)
        case IsTriggerArea::Store_BackToPlanning: {
            system_manager::store::move_purchased_furniture();

            GameState::get().transition_to_game();

            for (RefEntity player :
                 EQ(SystemManager::get().oldAll)
                     .whereType(EntityType::Player)
                     .whereInside(STORE_BUILDING.min(), STORE_BUILDING.max())
                     .gen()) {
                move_player_SERVER_ONLY(player, game::State::InGame);
            }
        } break;
    }
}

void update_dynamic_trigger_area_settings(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

    if (ita.type == IsTriggerArea::Unset) {
        log_warn("You created a trigger area without a type");
        TranslatableString not_configured_ts =
            TODO_TRANSLATE("Not Configured", TodoReason::UserFacingError);
        ita.update_title(not_configured_ts);
        return;
    }

    // These are the ones that should only change on language update
    switch (ita.type) {
        case IsTriggerArea::ModelTest_BackToLobby: {
            // TODO translate?
            ita.update_title(NO_TRANSLATE("Back To Lobby"));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Lobby_ModelTest: {
            ita.update_title(NO_TRANSLATE("Model Testing"));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Lobby_PlayGame: {
            ita.update_title(TranslatableString(strings::i18n::START_GAME));
            ita.update_subtitle(TranslatableString(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Store_BackToPlanning: {
            ita.update_title(
                TranslatableString(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            ita.update_subtitle(
                TranslatableString(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            return;
        } break;
        case IsTriggerArea::Store_Reroll: {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsRoundSettingsManager& irsm =
                sophie.get<IsRoundSettingsManager>();
            int reroll_cost = irsm.get<int>(ConfigKey::StoreRerollPrice);
            auto str =
                TranslatableString(strings::i18n::StoreReroll)
                    .set_param(strings::i18nParam::RerollCost, reroll_cost);

            ita.update_title(str);
            ita.update_subtitle(str);
            return;
        } break;
        case IsTriggerArea::Unset:
        case IsTriggerArea::Progression_Option1:
        case IsTriggerArea::Progression_Option2:
            break;
    }

    // These are the dynamic ones

    TranslatableString internal_ts =
        TODO_TRANSLATE("(internal)", TodoReason::UserFacingError);
    switch (ita.type) {
        case IsTriggerArea::Progression_Option1:  // fall through
        case IsTriggerArea::Progression_Option2: {
            // TODO this is happening every frame?
            ita.update_title(internal_ts);
            ita.update_subtitle(internal_ts);

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipm =
                sophie.get<IsProgressionManager>();

            if (!ipm.collectedOptions) {
                ita.update_title(internal_ts);
                ita.update_subtitle(internal_ts);
                return;
            }

            bool isOption1 = ita.type == IsTriggerArea::Progression_Option1;

            ita.update_title(ipm.get_option_title(isOption1));
            ita.update_subtitle(ipm.get_option_subtitle(isOption1));
            return;
        } break;
        default:
            break;
    }

    TranslatableString not_configured_ts =
        TODO_TRANSLATE("Not Configured", TodoReason::UserFacingError);

    ita.update_title(not_configured_ts);
    ita.update_subtitle(not_configured_ts);
    log_warn(
        "Trying to update trigger area title but type {} not handled "
        "anywhere",
        ita.type);
}

void process_trigger_area(Entity& entity, float dt) {
    update_dynamic_trigger_area_settings(entity, dt);
    count_all_possible_trigger_area_entrants(entity, dt);
    count_in_building_trigger_area_entrants(entity, dt);
    count_trigger_area_entrants(entity, dt);
    update_trigger_area_percent(entity, dt);
    trigger_cb_on_full_progress(entity, dt);
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

bool _create_nuxes(Entity&) {
    if (SystemManager::get().is_nighttime()) return false;

    OptEntity player = EQ(SystemManager::get().oldAll)
                           .whereType(EntityType::Player)
                           .gen_first();
    if (!player.has_value()) return false;
    OptEntity reg = EntityQuery().whereType(EntityType::Register).gen_first();
    if (!reg.has_value()) return false;

    int player_id = player->id;
    int register_id = reg->id;

    // Planning mode tutorial
    if (1) {
        // Find register
        {
            // move the register out to the Planning_SpawnArea
            {
                OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
                    IsFloorMarker::Type::Planning_SpawnArea);
                reg->get<Transform>().update(
                    vec::to3(purchase_area->get<Transform>().as2()));
            }

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .should_attach_to(player_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([register_id](const IsNux& inux) -> bool {
                    OptEntity reg =
                        EntityQuery().whereID(register_id).gen_first();
                    if (!reg.has_value()) return false;

                    // We have to do oldAll because players
                    // are not in the normal entity list
                    return EQ(SystemManager::get().oldAll)
                        .whereID(inux.entityID)
                        .whereInRange(reg->get<Transform>().as2(), 2.f)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Look for the Register",
                                            TodoReason::SubjectToChange));
        }

        // Grab register
        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerHandTruckInteract);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .should_attach_to(player_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([register_id](const IsNux& inux) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(inux.entityID)
                        .whereIsHoldingFurnitureID(register_id)
                        .has_values();
                })
                // TODO replace playerpickup with the actual control
                .set_content(
                    TODO_TRANSLATE(fmt::format("Grab it with [{}]", tex_name),
                                   TodoReason::SubjectToChange));
        }

        // Place register
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn(
                    [register_id, &entity](const IsNux&) -> bool {
                        return EntityQuery()
                            .whereID(register_id)
                            .whereIsNotBeingHeld()
                            .whereSnappedPositionMatches(entity)
                            .has_values();
                    })
                .set_ghost(EntityType::Register)
                .set_content(
                    TODO_TRANSLATE("Place it on the highlighted square",
                                   TodoReason::SubjectToChange));
        }

        // Find FFD
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([ffd_id, player_id](const IsNux&) -> bool {
                    OptEntity ffd = EntityQuery().whereID(ffd_id).gen_first();
                    if (!ffd.has_value()) return false;

                    // We have to do oldAll because players
                    // are not in the normal entity list
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereInRange(ffd->get<Transform>().as2(), 2.f)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    "You get all night to setup your pub.\nYou can "
                    "use the FastForward Box to skip ahead",
                    TodoReason::SubjectToChange));
        }

        // Use FFD
        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerDoWork);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([](const IsNux&) -> bool {
                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    return timer.pct() >= 0.5f;
                })
                .set_content(TODO_TRANSLATE(
                    fmt::format("Use [{}] to skip time", tex_name),
                    TodoReason::SubjectToChange));
        }
    }

    // During round tutorial
    if (1) {
        // Explain customer
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            entity.addComponent<IsNux>()
                .should_cleanup_on_parent_death()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    // Now that the customer exists, we can attach to it
                    auto customer = EntityQuery()
                                        .whereType(EntityType::Customer)
                                        .gen_first();
                    inux.should_attach_to(customer->id);
                })
                .set_completion_fn([](const IsNux& inux) -> bool {
                    if (inux.time_shown < 5.f) return false;

                    auto customer = EntityHelper::getEntityForID(inux.entityID);
                    if (!customer) return false;

                    AIWaitInQueue& ai_wiq = customer->get<AIWaitInQueue>();
                    return ai_wiq.line_wait.last_line_position == 0;
                })
                .set_content(TODO_TRANSLATE("This is a customer, they will "
                                            "wait in line, \nand once at "
                                            "the front will order a drink",
                                            TodoReason::SubjectToChange));
        }

        // Grab a cup
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto cups = EntityQuery()
                                    .whereType(EntityType::Cupboard)
                                    .gen_first();
                    inux.should_attach_to(cups->id);
                })
                .set_completion_fn([player_id](const IsNux&) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .has_values();
                })
                .set_content(
                    TODO_TRANSLATE("Grab a cup", TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto table =
                        EntityQuery().whereType(EntityType::Table).gen_first();
                    inux.should_attach_to(table->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    return EntityQuery()
                        // Instead of finding the exact table we marked,
                        // just find any table with a cup
                        .whereType(EntityType::Table)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Place it down on a table",
                                            TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto sodamach = EntityQuery()
                                        .whereType(EntityType::SodaMachine)
                                        .gen_first();
                    inux.should_attach_to(sodamach->id);
                })
                .set_completion_fn([player_id](const IsNux&) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereIsHoldingItemOfType(EntityType::SodaSpout)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Grab the soda wand",
                                            TodoReason::SubjectToChange));
        }

        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerDoWork);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto drink =
                        EntityQuery().whereType(EntityType::Drink).gen_first();
                    inux.should_attach_to(drink->id);
                })
                .set_completion_fn([](const IsNux& inux) -> bool {
                    return EntityQuery()
                        .whereID(inux.entityID)
                        .whereIsDrinkAndMatches(Drink::coke)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    fmt::format("Use [{}] to fill the cup with soda", tex_name),
                    TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    bool filled_cup_exists =
                        EntityQuery()
                            .whereIsDrinkAndMatches(Drink::coke)
                            .has_values();

                    bool player_holding_spout =
                        EQ(SystemManager::get().oldAll)
                            .whereType(EntityType::Player)
                            .whereHasComponent<CanHoldItem>()
                            .whereIsHoldingItemOfType(EntityType::SodaSpout)
                            .has_values();
                    return filled_cup_exists && player_holding_spout;
                })
                .set_on_trigger([](IsNux& inux) {
                    auto sodamach = EntityQuery()
                                        .whereType(EntityType::SodaMachine)
                                        .gen_first();
                    inux.should_attach_to(sodamach->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    // No players holding spouts anymore
                    return EQ(SystemManager::get().oldAll)
                        .whereType(EntityType::Player)
                        .whereIsHoldingItemOfType(EntityType::SodaSpout)
                        .is_empty();
                })
                .set_content(TODO_TRANSLATE("Place the soda wand back down",
                                            TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto reg = EntityQuery()
                                   .whereType(EntityType::Register)
                                   .gen_first();
                    inux.should_attach_to(reg->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    return EntityQuery()
                        .whereType(EntityType::Register)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .whereHeldItemMatches([](const Entity& item) {
                            if (!item.hasTag(EntityType::Drink)) return false;
                            return item.get<IsDrink>().matches_drink(
                                Drink::coke);
                        })
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    "Place it on the register to serve the customer",
                    TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool {
                    if (!GameState::get().is_game_like()) return false;

                    bool has_customers = EntityQuery()
                                             .whereType(EntityType::Customer)
                                             .has_values();
                    if (!has_customers) return false;

                    // TODO :DUPE: used as well for sophie checks
                    const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};
                    bool all_customers_at_gather =
                        EntityQuery()
                            .whereType(EntityType::Customer)
                            .whereNotInRange(endpos, TILESIZE * 2.f)
                            .is_empty();

                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    bool lt_halfway_through_day = timer.pct() <= 0.5f;

                    return all_customers_at_gather && lt_halfway_through_day;
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    // hide when 80% through the day
                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    return timer.pct() >= 0.8f;
                })
                .set_content(TODO_TRANSLATE("Since customers are all gone, \n"
                                            "Fast Forward to the next day",
                                            TodoReason::SubjectToChange));
        }

        // this is the upgrade room, you will either get a new recipe or a
        // new gimmic for your restaurant
        //
        // often new upgrades, unlock new furniture. for the ones that are
        // required, you will get one for free
        //
        // why is the "cant start until" showing so early in the day
    }

    log_info("created nuxes");
    return true;
}

void process_nux_updates(Entity& entity, float dt) {
    if (entity.is_missing<IsNuxManager>()) return;

    // Tutorial isnt on so dont do any nuxes
    if (!entity.get<IsRoundSettingsManager>()
             .interactive_settings.is_tutorial_active) {
        return;
    }

    // only generate the nux once you leave the lobby
    if (!GameState::get().is_game_like()) return;

    IsNuxManager& inm = entity.get<IsNuxManager>();
    if (!inm.initialized) {
        bool init = _create_nuxes(entity);
        if (!init) return;
        inm.initialized = init;
    }

    OptEntity active_nux = EntityQuery()
                               .whereHasComponent<IsNux>()
                               .whereLambda([](const Entity& entity) {
                                   return entity.get<IsNux>().is_active;
                               })
                               // there should only ever be one
                               .gen_first();

    // Process updates for current showing nux
    if (active_nux.has_value()) {
        Entity& nux = active_nux.asE();
        IsNux& inux = nux.get<IsNux>();

        inux.pass_time(dt);

        if (inux.whileShowing) inux.whileShowing(inux, dt);

        bool parent_died = false;
        if (inux.cleanup_on_parent_death && inux.entityID != -1) {
            auto exi = EntityHelper::getEntityForID(inux.entityID);
            parent_died = !exi.has_value();
        }

        if (parent_died || inux.isComplete(inux)) {
            nux.cleanup = true;
            inux.is_active = false;
            active_nux = {};
        }
    }

    // if that one is still active, nothing else to do
    if (active_nux.has_value() && active_nux->get<IsNux>().is_active) return;

    // find next active nux
    OptEntity next_active = EntityQuery()
                                .whereHasComponent<IsNux>()
                                .whereLambda([](const Entity& entity) {
                                    const IsNux& inux = entity.get<IsNux>();
                                    return inux.shouldTrigger(inux);
                                })
                                .gen_first();

    // if we found one, then make it active
    if (next_active.has_value()) {
        IsNux& inux = next_active->get<IsNux>();
        if (inux.onTrigger) inux.onTrigger(inux);
        inux.is_active = true;
    }
}

void run_timer(Entity& entity, float dt) {
    if (entity.is_missing<HasDayNightTimer>()) return;
    HasDayNightTimer& ht = entity.get<HasDayNightTimer>();

    ht.pass_time(dt);
    if (!ht.is_round_over()) return;

    if (ht.is_daytime()) {
        ht.start_night();

        if (entity.is_missing<IsBank>())
            log_warn("system_manager::run_timer missing IsBank");

        if (ht.days_until() <= 0) {
            IsBank& isbank = entity.get<IsBank>();
            if (isbank.balance() < ht.rent_due()) {
                log_error("you ran out of money, sorry");
            }
            isbank.withdraw(ht.rent_due());
            ht.reset_rent_days();
            // TODO update rent due amount
            ht.update_amount_due(ht.rent_due() + 25);

            // TODO add a way to pay ahead of time ?? design
        }

        return;
    }

    if (entity.is_missing<IsProgressionManager>())
        log_warn("system_manager::run_timer missing IsProgressionManager");
    system_manager::progression::collect_progression_options(entity, dt);

    // TODO theoretically we shouldnt start until after you choose upgrades but
    // we are gonna change how this works later anyway i think
    ht.start_day();
}

void process_squirter(Entity& entity, float dt) {
    if (!check_type(entity, EntityType::Squirter)) return;
    IsSquirter& is_squirter = entity.get<IsSquirter>();
    CanHoldItem& sqCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to squirt into
    if (sqCHI.empty()) {
        is_squirter.reset();
        is_squirter.set_drink_id(-1);
        return;
    }

    // cant squirt into this !
    if (sqCHI.item().is_missing<IsDrink>()) return;

    // so we got something, lets see if anyone around can give us
    // something to use

    auto pos = entity.get<Transform>().as2();
    OptEntity closest_furniture =
        EntityQuery()
            .whereHasComponentAndLambda<CanHoldItem>(
                [](const CanHoldItem& chi) {
                    if (chi.empty()) return false;
                    const Item& item = chi.const_item();
                    // TODO should we instead check for <AddsIngredient>?
                    if (!check_type(item, EntityType::Alcohol)) return false;
                    return true;
                })
            .whereInRange(pos, 1.25f)
            // NOTE: if you change this make sure that this always sorts the
            // same per game version
            .orderByDist(pos)
            .gen_first();

    if (!closest_furniture) {
        // Nothing anymore, probably someone took the item away
        // working reset
        return;
    }
    Entity& drink = sqCHI.item();
    Item& item = closest_furniture->get<CanHoldItem>().item();

    if (is_squirter.drink_id() == drink.id) {
        is_squirter.reset();
        return;
    }

    if (is_squirter.item_id() == -1) {
        is_squirter.update(item.id, closest_furniture->get<Transform>().pos());
    }
    vec3 item_position = is_squirter.picked_up_at();

    if (item.id != is_squirter.item_id()) {
        log_warn("squirter : item changed while working was {} now {}",
                 is_squirter.item_id(), item.id);
        is_squirter.reset();
        return;
    }

    if (vec::distance(vec::to2(item_position),
                      closest_furniture->get<Transform>().as2()) > 1.f) {
        log_warn("squirter : we matched something different {} {}",
                 item_position, closest_furniture->get<Transform>().as2());
        // We matched something different
        is_squirter.reset();
        return;
    }

    bool complete = is_squirter.pass_time(dt);
    if (!complete) {
        // keep going :)
        return;
    }

    is_squirter.set_drink_id(drink.id);

    bool cleanup = items::_add_item_to_drink_NO_VALIDATION(drink, item);
    if (cleanup) {
        closest_furniture->get<CanHoldItem>().update(nullptr, -1);
    }
}

void process_trash(Entity& entity, float) {
    if (!check_type(entity, EntityType::Trash)) return;

    CanHoldItem& trashCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to delete
    if (trashCHI.empty()) return;

    trashCHI.item().cleanup = true;
    trashCHI.update(nullptr, -1);
}

void process_pnumatic_pipe_pairing(Entity& entity, float) {
    if (entity.is_missing<IsPnumaticPipe>()) return;

    IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();

    if (ipp.has_pair()) return;

    OptEntity other_pipe = EntityQuery()  //
                               .whereNotMarkedForCleanup()
                               .whereNotID(entity.id)
                               .whereHasComponent<IsPnumaticPipe>()
                               .whereLambda([](const Entity& pipe) {
                                   const IsPnumaticPipe& otherpp =
                                       pipe.get<IsPnumaticPipe>();
                                   // Find only the ones that dont have a
                                   // pair
                                   return !otherpp.has_pair();
                               })
                               .gen_first();

    if (other_pipe.has_value()) {
        IsPnumaticPipe& otherpp = other_pipe->get<IsPnumaticPipe>();
        otherpp.paired_id = entity.id;
        ipp.paired_id = other_pipe->id;
    }

    // still dont have a pair, we probably just have an odd number
    if (!ipp.has_pair()) return;
}
void process_pnumatic_pipe_movement(Entity& entity, float) {
    if (entity.is_missing<IsPnumaticPipe>()) return;

    IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();
    const CanHoldItem& chi = entity.get<CanHoldItem>();

    if (chi.empty()) {
        ipp.item_id = -1;
        ipp.recieving = false;
        return;
    }

    int cur_id = chi.const_item().id;
    ipp.item_id = cur_id;
}

void reset_customers_that_need_resetting(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return;
    CanOrderDrink& cod = entity.get<CanOrderDrink>();

    if (cod.order_state != CanOrderDrink::OrderState::NeedsReset) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    {
        int max_num_orders =
            // max() here to avoid a situation where we get 0 after an
            // upgrade
            (int) fmax(1, irsm.get<int>(ConfigKey::MaxNumOrders));

        cod.reset_customer(max_num_orders,
                           progressionManager.get_random_unlocked_drink());
    }

    {
        // Set the patience based on how many ingredients there are
        // TODO add a map of ingredient to how long it probably takes to
        // make

        auto ingredients = get_req_ingredients_for_drink(cod.get_order());
        float patience_multiplier =
            irsm.get<float>(ConfigKey::PatienceMultiplier);
        entity.get<HasPatience>().update_max(ingredients.count() * 30.f *
                                             patience_multiplier);
        entity.get<HasPatience>().reset();
    }
}

// TODO this sucks, but we dont have a way to have local clientside
// animtions for data thats on the backend yet
void pass_time_for_transaction_animation(Entity& entity, float dt) {
    if (entity.is_missing<IsBank>()) return;
    IsBank& bank = entity.get<IsBank>();

    std::vector<IsBank::Transaction>& transactions = bank.get_transactions();

    // Remove any old ones
    remove_all_matching<IsBank::Transaction>(
        transactions, [](const IsBank::Transaction& transaction) {
            return transaction.remainingTime <= 0.f || transaction.amount == 0;
        });

    if (transactions.empty()) return;

    IsBank::Transaction& transaction = bank.get_next_transaction();
    transaction.remainingTime -= dt;
}

void pop_out_when_colliding(Entity& entity, float) {
    const auto no_clip_on =
        GLOBALS.get_or_default<bool>("no_clip_enabled", false);
    if (no_clip_on) return;

    // Only popping out players right now
    if (!check_type(entity, EntityType::Player)) return;

    OptEntity match =  //
        EntityQuery()
            .whereNotID(entity.id)  // not us
            .whereHasComponent<IsSolid>()
            .whereInRange(entity.get<Transform>().as2(), 0.7f)
            .whereLambdaExistsAndTrue([](const Entity& other) {
                // this filter isnt for you
                if (other.is_missing<CanBeHeld_HT>()) return true;
                // ignore if held
                return !other.get<CanBeHeld_HT>().is_held();
            })
            .include_store_entities()
            .gen_first();

    if (!match) {
        return;
    }

    const CanHoldHandTruck& chht = entity.get<CanHoldHandTruck>();
    if (chht.is_holding()) {
        OptEntity hand_truck =
            EntityHelper::getEntityForID(chht.hand_truck_id());
        if (match->id == chht.hand_truck_id()) {
            return;
        }
        if (chht.is_holding() &&
            match->id == hand_truck->get<CanHoldFurniture>().furniture_id()) {
            return;
        }
    }

    const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
    if (chf.is_holding_furniture()) return;
    if (chf.furniture_id() == match->id) return;

    vec2 new_position = entity.get<Transform>().as2();

    int i = static_cast<int>(new_position.x);
    int j = static_cast<int>(new_position.y);
    for (int a = 0; a < 8; a++) {
        auto position = (vec2{(float) i + (bfs::neigh_x[a]),
                              (float) j + (bfs::neigh_y[a])});
        if (EntityHelper::isWalkable(position)) {
            new_position = position;
            break;
        }
    }

    entity.get<Transform>().update(vec::to3(new_position));
}

namespace store {

void cart_management(Entity& entity, float) {
    if (!check_type(entity, EntityType::FloorMarker)) return;
    if (entity.is_missing<IsFloorMarker>()) return;
    const IsFloorMarker& ifm = entity.get<IsFloorMarker>();
    if (ifm.type != IsFloorMarker::Store_PurchaseArea) return;

    int amount_in_cart = 0;

    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;

        // Its free!
        if (marked_entity->has<IsFreeInStore>()) continue;
        // it was already purchased or is otherwise just randomly in the store
        if (marked_entity->is_missing<IsStoreSpawned>()) continue;

        amount_in_cart += std::max(
            0, get_price_for_entity_type(get_entity_type(marked_entity.asE())));
    }

    OptEntity sophie = EntityQuery().whereType(EntityType::Sophie).gen_first();
    if (sophie.valid()) {
        IsBank& bank = sophie->get<IsBank>();
        bank.update_cart(amount_in_cart);
    }

    // Hack to force the validation function to run every frame
    OptEntity purchase_area = EntityHelper::getMatchingTriggerArea(
        IsTriggerArea::Type::Store_BackToPlanning);
    if (purchase_area.valid()) {
        (void) purchase_area->get<IsTriggerArea>().should_progress();
    }
}

void move_purchased_furniture() {
    // Grab the overlap area so we can see what it marked
    OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Store_PurchaseArea);
    const IsFloorMarker& ifm = purchase_area->get<IsFloorMarker>();

    // Grab the plannig spawn area so we can place in the right spot
    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_SpawnArea);
    vec3 spawn_position = spawn_area->get<Transform>().pos();

    OptEntity sophie = EntityQuery().whereType(EntityType::Sophie).gen_first();
    VALIDATE(sophie.valid(), "sophie should exist when moving furniture");
    IsBank& bank = sophie->get<IsBank>();

    int amount_in_cart = 0;

    // for every marked, move them over
    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;

        // Move it to the new spot
        Transform& transform = marked_entity->get<Transform>();
        transform.update(spawn_position);
        transform.update_y(0);

        // Remove the 'store_cleanup' marker
        if (marked_entity->has<IsStoreSpawned>()) {
            marked_entity->removeComponent<IsStoreSpawned>();
        }

        // Some items can hold other items, we should move that item too
        // they arent being caught by the marker since we only mark
        // solid items
        system_manager::update_held_item_position(marked_entity.asE(), 0.f);

        // Its not free!
        if (marked_entity->is_missing<IsFreeInStore>()) {
            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }
    }

    VALIDATE(amount_in_cart == bank.cart(),
             "Amount we computed for in cart should always match the actual "
             "amount in our cart")

    // Commit the withdraw
    bank.withdraw(amount_in_cart);
    bank.update_cart(0);
}

}  // namespace store

}  // namespace system_manager

void SystemManager::on_game_state_change(game::State new_state,
                                         game::State old_state) {
    log_warn("system manager on gamestate change from {} to {}", old_state,
             new_state);

    transitions.emplace_back(std::make_pair(old_state, new_state));
}

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

        // NOTE: System updates are now handled by afterhours systems:
        // - ModelTestUpdateSystem (runs when State::ModelTest)
        // - InRoundUpdateSystem (runs when is_game_like() && is_nighttime())
        // - PlanningUpdateSystem (runs when is_game_like() && is_daytime())
        // - GameLikeUpdateSystem (runs when is_game_like())
        // - SixtyFpsUpdateSystem (runs in all states)
        // All systems have should_run() methods that match the original
        // conditional logic, so we no longer need the conditional blocks here.
        every_frame_update(entities, dt);
        process_state_change(entities, dt);

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

void SystemManager::process_state_change(
    const std::vector<std::shared_ptr<Entity>>&, float) {
    if (transitions.empty()) return;

    for (const auto& transition : transitions) {
        const auto [old_state, new_state] = transition;
        // We just always ignore paused transitions since
        // the game state didnt actually change in a meaningful way
        if (old_state == game::State::Paused) continue;
        if (new_state == game::State::Paused) continue;
    }

    transitions.clear();
}

void SystemManager::every_frame_update(const Entities& entity_list, float) {
    PathRequestManager::process_responses(entity_list);

    // for_each(entity_list, dt, [](Entity& entity, float dt) {});
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

bool SystemManager::is_daytime() const {
    for (const auto& entity : oldAll) {
        if (entity->is_missing<HasDayNightTimer>()) continue;
        return entity->get<HasDayNightTimer>().is_daytime();
    }
    return false;
}
bool SystemManager::is_nighttime() const { return !is_daytime(); }

bool SystemManager::is_some_player_near(vec2 spot, float distance) const {
    bool someone_close = false;
    for (auto& player : remote_players) {
        auto pos = player->get<Transform>().as2();
        if (vec::distance(pos, spot) < distance) {
            someone_close = true;
            break;
        }
    }
    return someone_close;
}
