

#include "system_manager.h"

///
#include "../components/adds_ingredient.h"
#include "../components/base_component.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_held.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_be_pushed.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/debug_name.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_name.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_drink.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_rotatable.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_user_input.h"
#include "../components/shows_progress_bar.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../engine/util.h"
#include "sophie.h"
///
#include "../engine/pathfinder.h"
#include "../engine/tracy.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../map.h"
#include "../network/server.h"
#include "ingredient_helper.h"
#include "input_process_manager.h"
#include "job_system.h"
#include "magic_enum/magic_enum.hpp"
#include "progression.h"
#include "rendering_system.h"
#include "ui_rendering_system.h"

extern ui::UITheme UI_THEME;

namespace system_manager {

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
            position = {LOBBY_ORIGIN, 0, 0};
        } break;
        case game::InRound:  // fall through
        case game::Planning: {
            position = {0, 0, 0};
        } break;
        case game::Progression: {
            position = {PROGRESSION_ORIGIN, 0, 0};
        } break;
        case game::Store: {
            position = {STORE_ORIGIN, 0, 0};
        } break;
        case game::ModelTest: {
            position = {MODEL_TEST_ORIGIN, 0, 0};
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
    ifm.clear();

    for (const auto& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (e->id == entity.id) continue;
        if (check_type(*e, EntityType::Player)) continue;
        if (check_type(*e, EntityType::RemotePlayer)) continue;
        if (check_type(*e, EntityType::SodaSpout)) continue;
        if (e->is_missing<IsSolid>()) continue;

        if (CheckCollisionBoxes(
                e->get<Transform>().bounds(),
                entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))) {
            log_trace(" FloorMarker marking {} {}", e->get<DebugName>().name(),
                      e->get<DebugName>().get_type());
            ifm.mark(e->id);
        }
    }
}

// TODO if cannot be placed in this spot make it obvious to the user
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
            new_pos.y += TILESIZE / 2;
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

    can_hold_item.item()->get<Transform>().update(new_pos);
}

void reset_highlighted(Entity& entity, float) {
    if (entity.is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity.get<CanBeHighlighted>();
    cbh.update(entity, false);
}

void highlight_facing_furniture(Entity& entity, float) {
    if (entity.is_missing<CanHighlightOthers>()) return;
    const Transform& transform = entity.get<Transform>();
    // TODO add a player reach component
    const CanHighlightOthers& cho = entity.get<CanHighlightOthers>();

    OptEntity match = EntityHelper::getClosestMatchingFurniture(
        transform, cho.reach(),
        [](Entity& e) { return e.template has<CanBeHighlighted>(); });
    if (!match) return;

    match->get<CanBeHighlighted>().update(entity, true);
}

// TODO We need like a temporary storage for this
void move_entity_based_on_push_force(Entity& entity, float, vec3& new_pos_x,
                                     vec3& new_pos_z) {
    CanBePushed& cbp = entity.get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force().x;
    cbp.update_x(0.0f);

    new_pos_z.z += cbp.pushed_force().z;
    cbp.update_z(0.0f);
}

void process_conveyer_items(Entity& entity, float dt) {
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing_any<CanHoldItem, ConveysHeldItem, CanBeTakenFrom>())
        return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    CanBeTakenFrom& canBeTakenFrom = entity.get<CanBeTakenFrom>();
    ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

    // we are not holding anything
    if (canHold.empty()) return;

    // make sure no one can insta-grab from us
    canBeTakenFrom.update(false);

    // if the item is less than halfway, just keep moving it along
    // 0 is halfway btw
    if (conveysHeldItem.relative_item_pos <= 0.f) {
        conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
        return;
    }

    bool is_ipp = entity.has<IsPnumaticPipe>();

    const auto _conveyer_filter = [&entity, &canHold](Entity& furn) -> bool {
        // cant be us
        if (entity.id == furn.id) return false;
        // needs to be able to hold something
        if (furn.is_missing<CanHoldItem>()) return false;
        const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
        // has to be empty
        if (furnCHI.is_holding_item()) return false;
        // can this furniture hold the item we are passing?
        // some have filters
        bool can_hold =
            furnCHI.can_hold(*(canHold.item()), RespectFilter::ReqOnly);

        return can_hold;
    };

    const auto _ipp_filter = [&entity, _conveyer_filter](Entity& furn) -> bool {
        // if we are a pnumatic pipe, filter only down to our guy
        if (furn.is_missing<IsPnumaticPipe>()) return false;
        const IsPnumaticPipe& mypp = entity.get<IsPnumaticPipe>();
        if (mypp.paired_id != furn.id) return false;
        if (mypp.recieving) return false;
        return _conveyer_filter(furn);
    };

    OptEntity match = is_ipp
                          ? EntityHelper::getClosestMatchingEntity(
                                transform.as2(), MAX_SEARCH_RANGE, _ipp_filter)
                          : EntityHelper::getClosestMatchingFurniture(
                                transform, 1.f, _conveyer_filter);

    // no match means we can't continue, stay in the middle
    if (!match) {
        conveysHeldItem.relative_item_pos = 0.f;
        canBeTakenFrom.update(true);
        return;
    }

    if (is_ipp) {
        entity.get<IsPnumaticPipe>().recieving = false;
    }

    // we got something that will take from us,
    // but only once we get close enough

    // so keep moving forward
    if (conveysHeldItem.relative_item_pos <= ConveysHeldItem::ITEM_END) {
        conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
        return;
    }

    // we reached the end, pass ownership

    CanHoldItem& ourCHI = entity.get<CanHoldItem>();

    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    matchCHI.update(ourCHI.item(), entity.id);

    ourCHI.update(nullptr, -1);

    canBeTakenFrom.update(true);  // we are ready to have someone grab from us
    // reset so that the next item we get starts from beginning
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

    if (match->has<CanBeTakenFrom>()) {
        match->get<CanBeTakenFrom>().update(false);
    }

    if (is_ipp && match->has<IsPnumaticPipe>()) {
        match->get<IsPnumaticPipe>().recieving = true;
    }

    // TODO if we are pushing onto a conveyer, we need to make sure
    // we are keeping track of the orientations
    //
    //  --> --> in this case we want to place at 0.5f
    //
    //          ^
    //    -->-> |     in this we want to place at 0.f instead of -0.5
}

// TODO This function is 33% of our run time
// getMatchingEntity is a pretty large chunk of that
// processconveyer is 10x faster ...
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
                *(furnCHI.const_item()), RespectFilter::All);

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

    ourCHI.update(matchCHI.item(), entity.id);
    matchCHI.update(nullptr, -1);

    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
}

void process_grabber_filter(Entity& entity, float) {
    if (!check_type(entity, EntityType::FilteredGrabber)) return;
    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // If we are holding something, then:
    // - either its already in the filter (and setting it wont be a big deal)
    // - or we should set the filter

    EntityFilter& ef = canHold.get_filter();
    ef.set_filter_with_entity(*(canHold.const_item()));
}

template<typename... TArgs>
void backfill_empty_container(const EntityType& match_type, Entity& entity,
                              TArgs&&... args) {
    if (entity.is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity.get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    Entity& item =
        EntityHelper::createItem(iic.type(), std::forward<TArgs>(args)...);

    canHold.update(EntityHelper::getEntityAsSharedPtr(item), entity.id);
}

void process_is_container_and_should_backfill_item(Entity& entity, float) {
    if (entity.is_missing<IsItemContainer>()) return;
    const IsItemContainer& iic = entity.get<IsItemContainer>();

    if (entity.is_missing<CanHoldItem>()) return;
    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    auto pos = entity.get<Transform>().as2();

    if (iic.should_use_indexer() && entity.has<Indexer>()) {
        Indexer& indexer = entity.get<Indexer>();

        // TODO :backfill-correct: This should match whats in container_haswork

        // TODO For now we are okay doing this because the other indexer
        // (alcohol) always unlocks rum first which is index 0. if that changes
        // we gotta update this
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
        canHold.item()->cleanup = true;
        canHold.update(nullptr, -1);
    }

    auto pos = entity.get<Transform>().as2();
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
    int item_value = canHold.item()->get<HasSubtype>().get_type_index();

    if (current_value != item_value) {
        canHold.item()->cleanup = true;
        canHold.update(nullptr, -1);
    }
}

void handle_autodrop_furniture_when_exiting_planning(Entity& entity) {
    if (entity.is_missing<CanHoldFurniture>()) return;
    const CanHoldFurniture& ourCHF = entity.get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // TODO need to find a spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

void release_mop_buddy_at_start_of_day(Entity& entity) {
    if (!check_type(entity, EntityType::MopBuddyHolder)) return;

    CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.empty()) return;

    // grab yaboi
    std::shared_ptr<Item> item = chi.item();

    // let go of the item
    item->get<IsItem>().set_held_by(EntityType::Unknown, -1);
    chi.update(nullptr, -1);
}

void delete_trash_when_leaving_planning(Entity& entity) {
    if (!check_type(entity, EntityType::FloorMarker)) return;
    if (entity.is_missing<IsFloorMarker>()) return;
    const IsFloorMarker& ifm = entity.get<IsFloorMarker>();
    if (ifm.type != IsFloorMarker::Planning_TrashArea) return;

    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;
        marked_entity->cleanup = true;

        // Also delete the held item
        CanHoldItem& markedCHI = marked_entity->get<CanHoldItem>();
        if (!markedCHI.empty()) {
            markedCHI.item()->cleanup = true;
            markedCHI.update(nullptr, -1);
        }
    }
}

void delete_customers_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return;
    if (!check_type(entity, EntityType::Customer)) return;

    entity.cleanup = true;
}

void reset_toilet_when_leaving_inround(Entity& entity) {
    // TODO :DESIGN: do we actually want to do this?
    if (entity.is_missing<IsToilet>()) return;

    IsToilet& istoilet = entity.get<IsToilet>();
    istoilet.reset();
}

void delete_floating_items_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsItem>()) return;

    const IsItem& ii = entity.get<IsItem>();

    // Its being held by something so we'll get it in the function below
    if (ii.is_held()) return;

    // mark it for cleanup
    entity.cleanup = true;
}

void delete_held_items_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // Mark it as deletable
    const std::shared_ptr<Item>& item = canHold.item();

    // let go of the item
    item->cleanup = true;
    canHold.update(nullptr, -1);
}

void reset_max_gen_when_after_deletion(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<IsItemContainer>()) return;

    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    // If something wasnt deleted, then just ignore it for now
    if (canHold.is_holding_item()) return;

    entity.get<IsItemContainer>().reset_generations();
}

void refetch_dynamic_model_names(Entity& entity, float) {
    if (entity.is_missing<ModelRenderer>()) return;
    if (entity.is_missing<HasDynamicModelName>()) return;

    const HasDynamicModelName& hDMN = entity.get<HasDynamicModelName>();
    ModelRenderer& renderer = entity.get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch(entity));
}

void count_max_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (const auto& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (!check_type(*e, EntityType::Player)) continue;
        count++;
    }
    entity.get<IsTriggerArea>().update_max_entrants(count);
}

void count_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (const auto& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (!check_type(*e, EntityType::Player)) continue;
        if (CheckCollisionBoxes(
                e->get<Transform>().bounds(),
                entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))) {
            count++;
        }
    }
    entity.get<IsTriggerArea>().update_entrants(count);
}

void update_trigger_area_percent(Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

    ita.should_wave() ?  //
        ita.increase_progress(dt)
                      : ita.decrease_progress(dt);
}

void __spawn_machines_for_newly_unlocked_drink(IsProgressionManager& ipm,
                                               Drink option) {
    // today we dont have a way to statically know which machines
    // provide which ingredients because they are dynamic
    IngredientBitSet possibleNewIGs = get_req_ingredients_for_drink(option);

    // Because prereqs are handled, we dont need do check for them and can
    // assume that those bits will handle checking for it

    OptEntity spawn_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            // Note we spawn free items in the purchase area so its more obvious
            // that they are free
            return fm.type == IsFloorMarker::Type::Store_PurchaseArea;
        });

    if (!spawn_area) {
        // TODO need to guarantee this exists long before we get here
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
        if (IngredientHelper::has_machines_required_for_ingredient(ig)) return;

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
            case Rum:
            case Tequila:
            case Vodka:
            case Whiskey:
            case Gin:
            case TripleSec:
            case Bitters: {
                int alc_index =
                    index_of<Ingredient, ingredient::Alcohols.size()>(
                        ingredient::Alcohols, ig);
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
            // TODO implement for these once thye have spawners
            case Salt:
            case MintLeaf:
            case Invalid:
                break;
        }
    });
}

void trigger_cb_on_full_progress(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    const IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;

    const auto _choose_option = [](int option_chosen) {
        GameState::get().transition_to_store();

        SystemManager::get().for_each_old([](Entity& e) {
            if (check_type(e, EntityType::Player)) {
                move_player_SERVER_ONLY(e, game::State::Store);
                return;
            }
        });

        {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            IsProgressionManager& ipm = sophie.get<IsProgressionManager>();
            // choose given option

            // reset options so it collections new ones next upgrade round
            ipm.collectedOptions = false;

            Drink option = option_chosen == 0 ? ipm.option1 : ipm.option2;

            // Mark the drink unlocked
            ipm.unlock_drink(option);
            // Unlock any igredients it needs
            auto possibleNewIGs = get_req_ingredients_for_drink(option);
            bitset_utils::for_each_enabled_bit(
                possibleNewIGs, [&ipm](size_t index) {
                    Ingredient ig = magic_enum::enum_value<Ingredient>(index);
                    ipm.unlock_ingredient(ig);
                });

            __spawn_machines_for_newly_unlocked_drink(ipm, option);
        }
    };

    switch (ita.type) {
        case IsTriggerArea::Unset:
            break;
        case IsTriggerArea::ModelTest_BackToLobby: {
            GameState::get().transition_to_lobby();
            {
                // TODO add a tagging system so we can delete certain sets of
                // ents
                float rad = 30;
                const auto ents = EntityHelper::getAllInRange(
                    {MODEL_TEST_ORIGIN - rad, -1.f * rad},
                    {MODEL_TEST_ORIGIN + rad, rad});

                for (Entity& to_delete : ents) {
                    // TDOO add a way to skip the permananent ones
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
            GameState::get().transition_to_planning();
            SystemManager::get().for_each_old([](Entity& e) {
                if (!check_type(e, EntityType::Player)) return;
                move_player_SERVER_ONLY(e, game::State::Planning);
            });
        } break;
        case IsTriggerArea::Progression_Option1:
            _choose_option(0);
            break;
        case IsTriggerArea::Progression_Option2:
            _choose_option(1);
            break;
        case IsTriggerArea::Store_BackToPlanning: {
            GameState::get().transition_to_planning();
            SystemManager::get().for_each_old([](Entity& e) {
                if (!check_type(e, EntityType::Player)) return;
                move_player_SERVER_ONLY(e, game::State::Planning);
            });
        } break;
    }
}

void update_dynamic_trigger_area_settings(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();

    if (ita.type == IsTriggerArea::Unset) {
        log_warn("You created a trigger area without a type");
        ita.update_title("Not Configured");
        return;
    }

    // These are the ones that should only change on language update
    switch (ita.type) {
        case IsTriggerArea::ModelTest_BackToLobby: {
            ita.update_title("Back To Lobby");
            ita.update_subtitle(text_lookup(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Lobby_ModelTest: {
            ita.update_title("Model Testing");
            ita.update_subtitle(text_lookup(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Lobby_PlayGame: {
            ita.update_title(text_lookup(strings::i18n::START_GAME));
            ita.update_subtitle(text_lookup(strings::i18n::LOADING));
            return;
        } break;
        case IsTriggerArea::Store_BackToPlanning: {
            ita.update_title(
                text_lookup(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            ita.update_subtitle(
                text_lookup(strings::i18n::TRIGGERAREA_PURCHASE_FINISH));
            return;
        } break;
        case IsTriggerArea::Unset:
        case IsTriggerArea::Progression_Option1:
        case IsTriggerArea::Progression_Option2:
            break;
    }

    // These are the dynamic ones

    switch (ita.type) {
        case IsTriggerArea::Progression_Option1:  // fall through
        case IsTriggerArea::Progression_Option2: {
            if (GameState::get().is_not(game::State::Progression)) {
                ita.update_title("(internal)");
                ita.update_subtitle("(internal)");
                return;
            }

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipm =
                sophie.get<IsProgressionManager>();

            if (!ipm.collectedOptions) {
                ita.update_title("(internal)");
                ita.update_subtitle("(internal)");
                return;
            }

            Drink option = ita.type == IsTriggerArea::Progression_Option1
                               ? ipm.option1
                               : ipm.option2;
            ita.update_title(
                fmt::format("{}", magic_enum::enum_name<Drink>(option)));
            ita.update_subtitle(
                fmt::format("{}", magic_enum::enum_name<Drink>(option)));
            return;
        } break;
        default:
            break;
    }

    ita.update_title("Not Configured");
    ita.update_subtitle("Not Configured");
    log_warn(
        "Trying to update trigger area title but type {} not handled "
        "anywhere",
        ita.type);
}

void process_trigger_area(Entity& entity, float dt) {
    update_dynamic_trigger_area_settings(entity, dt);
    count_max_trigger_area_entrants(entity, dt);
    count_trigger_area_entrants(entity, dt);
    update_trigger_area_percent(entity, dt);
    trigger_cb_on_full_progress(entity, dt);
}

void process_spawner(Entity& entity, float dt) {
    if (entity.is_missing<IsSpawner>()) return;
    vec2 pos = entity.get<Transform>().as2();

    IsSpawner& iss = entity.get<IsSpawner>();

    bool is_time_to_spawn = iss.pass_time(dt);
    if (!is_time_to_spawn) return;

    SpawnInfo info{
        .location = pos,
        .is_first_this_round = (iss.get_num_spawned() == 0),
    };

    // If there is a validation function check that first
    bool can_spawn_here_and_now = iss.validate(entity, info);
    if (!can_spawn_here_and_now) return;

    bool should_prev_dupes = iss.prevent_dupes();
    if (should_prev_dupes) {
        for (const Entity& e : EntityHelper::getEntitiesInPosition(pos)) {
            if (e.id == entity.id) continue;

            // Other than invalid and Us, is there anything else there?
            // log_info(
            // "was ready to spawn but then there was someone there
            // already");
            return;
        }
    }

    auto& new_ent = EntityHelper::createEntity();
    iss.spawn(new_ent, info);
    iss.post_spawn_reset();

    if (iss.has_spawn_sound()) {
        network::Server::play_sound(pos, iss.get_spawn_sound());
    }
}

void run_timer(Entity& entity, float dt) {
    if (entity.is_missing<HasTimer>()) return;
    HasTimer& ht = entity.get<HasTimer>();

    ht.pass_time(dt);

    // If we round isnt over yet, then thats all for now
    if (ht.round_not_over()) return;

    // Round is over so pass time for the round switch countdown

    ht.pass_time_round_switch(dt);

    // player still has time to do things
    if (ht.round_switch_not_ready()) return;

    // the timer expired, time to switch rounds
    // but first need to check if we can or not

    // Round is actually over, reset timers

    switch (GameState::get().read()) {
        case game::State::Planning: {
            GameState::get().set(game::State::InRound);
        } break;
        case game::State::InRound: {
            GameState::get().set(game::State::Progression);
            SystemManager::get().for_each_old([](Entity& e) {
                if (check_type(e, EntityType::Player)) {
                    move_player_SERVER_ONLY(e, game::State::Progression);
                    return;
                }
            });
        } break;
        default:
            log_warn("processing round switch timer but no state handler {}",
                     GameState::get().read());
            return;
    }

    ht.reset_round_switch_timer().reset_timer();
}

void reset_empty_work_furniture(Entity& entity, float) {
    if (entity.is_missing<HasWork>()) return;
    if (entity.is_missing<CanHoldItem>()) return;

    HasWork& hasWork = entity.get<HasWork>();
    if (!hasWork.should_reset_on_empty()) return;

    const CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.empty()) {
        hasWork.reset_pct();
        return;
    }

    // if its not empty, we have to see if its an item that can be
    // worked
}

void process_has_rope(Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<HasRopeToItem>()) return;

    HasRopeToItem& hrti = entity.get<HasRopeToItem>();

    // No need to have rope if spout is put away
    const CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.is_holding_item()) {
        hrti.clear();
        return;
    }

    // Find the player who is holding __OUR__ spout

    OptEntity player;
    for (const std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
        if (!e) continue;
        // only route to players
        if (!check_type(*e, EntityType::Player)) continue;
        auto i = e->get<CanHoldItem>().item();
        if (!i) continue;
        // that are holding spouts
        if (!check_type(*i, EntityType::SodaSpout)) continue;
        // that match the one we were holding
        if (i->id != chi.last_id()) continue;
        player = *e;
    }
    if (!player) return;

    auto pos = player->get<Transform>().as2();

    // If we moved more then regenerate
    if (vec::distance(pos, hrti.goal()) > TILESIZE) {
        hrti.clear();
    }

    // Already generated
    if (hrti.was_generated()) return;

    auto new_path = pathfinder::find_path(entity.get<Transform>().as2(), pos,
                                          [](vec2) { return true; });

    std::vector<vec2> extended_path;
    std::optional<vec2> prev;
    for (auto p : new_path) {
        if (prev.has_value()) {
            extended_path.push_back(vec::lerp(prev.value(), p, 0.33f));
            extended_path.push_back(vec::lerp(prev.value(), p, 0.66f));
            extended_path.push_back(vec::lerp(prev.value(), p, 0.99f));
        }
        extended_path.push_back(p);
        prev = p;
    }

    for (auto p : extended_path) {
        Entity& item = EntityHelper::createItem(EntityType::SodaSpout, p);
        item.get<IsItem>().set_held_by(EntityType::Player, player->id);
        item.addComponent<IsSolid>();
        hrti.add(item);
    }
    hrti.mark_generated(pos);
}

// TODO its been a really long time since i tested this
// i feel like probably the enttity searching might not work the way
// people expect since the closest code isnt really good
void process_squirter(Entity& entity, float) {
    if (!check_type(entity, EntityType::Squirter)) return;

    CanHoldItem& sqCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to squirt into
    if (sqCHI.empty()) return;

    // cant squirt into this !
    if (sqCHI.item()->is_missing<IsDrink>()) return;

    // so we got something, lets see if anyone around can give us something
    // to use

    OptEntity closest_furniture = EntityHelper::getClosestMatchingEntity(
        entity.get<Transform>().as2(), 1.25f, [](const Entity& f) {
            if (f.is_missing<CanHoldItem>()) return false;
            const CanHoldItem& fchi = f.get<CanHoldItem>();
            if (fchi.empty()) return false;

            std::shared_ptr<Item> item = fchi.const_item();

            // TODO should we instead check for <AddsIngredient>?
            if (!check_type(*item, EntityType::Alcohol)) return false;
            return true;
        });
    if (!closest_furniture) return;

    std::shared_ptr<Entity> drink = sqCHI.item();
    std::shared_ptr<Item> item = closest_furniture->get<CanHoldItem>().item();

    bool cleanup = items::_add_item_to_drink_NO_VALIDATION(*drink, *item);
    if (cleanup) {
        closest_furniture->get<CanHoldItem>().update(nullptr, -1);
    }
}

void process_trash(Entity& entity, float) {
    if (!check_type(entity, EntityType::Trash)) return;

    CanHoldItem& trashCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to delete
    if (trashCHI.empty()) return;

    trashCHI.item()->cleanup = true;
    trashCHI.update(nullptr, -1);
}

void process_pnumatic_pipe_pairing(Entity& entity, float) {
    if (entity.is_missing<IsPnumaticPipe>()) return;

    IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();

    if (ipp.has_pair()) return;

    for (Entity& other : EntityHelper::getAllWithComponent<IsPnumaticPipe>()) {
        if (other.cleanup) continue;
        if (other.id == entity.id) continue;
        IsPnumaticPipe& otherpp = other.get<IsPnumaticPipe>();
        if (otherpp.has_pair()) continue;

        otherpp.paired_id = entity.id;
        ipp.paired_id = other.id;
        break;
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

    int cur_id = chi.const_item()->id;
    ipp.item_id = cur_id;
}

void increment_day_count(Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;
    entity.get<HasTimer>().dayCount++;
}

void reset_customer_spawner_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsSpawner>()) return;
    entity.get<IsSpawner>().reset_num_spawned();
}

void reset_register_queue_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<HasWaitingQueue>()) return;
    HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
    hwq.clear();
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
            irsm.get<int>(IsRoundSettingsManager::Config::Key::MaxNumOrders);
        cod.num_orders_rem = randIn(1, max_num_orders);

        cod.num_orders_had = 0;
        // If we have a forced order use that otherwise grab a random unlocked
        // drink
        cod.current_order = cod.forced_first_order.value_or(
            progressionManager.get_random_unlocked_drink());
        cod.forced_first_order = {};
        cod.order_state = CanOrderDrink::OrderState::Ordering;
    }

    {
        // Set the patience based on how many ingredients there are
        // TODO add a map of ingredient to how long it probably takes to make

        auto ingredients = get_req_ingredients_for_drink(cod.current_order);
        float patience_multiplier = irsm.get<float>(
            IsRoundSettingsManager::Config::Key::PatienceMultiplier);
        entity.get<HasPatience>().update_max(ingredients.count() * 30.f *
                                             patience_multiplier);
        entity.get<HasPatience>().reset();
    }
}

void update_new_max_customers(Entity& entity, float) {
    if (entity.is_missing<HasProgression>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    const HasTimer& hasTimer = sophie.get<HasTimer>();
    const int day_count = hasTimer.dayCount;

    if (check_type(entity, EntityType::CustomerSpawner)) {
        float customer_spawn_multiplier = irsm.get<float>(
            IsRoundSettingsManager::Config::Key::CustomerSpawnMultiplier);
        float round_length =
            irsm.get<float>(IsRoundSettingsManager::Config::Key::RoundLength);
        // TODO come up with a function to use here
        const int new_total =
            (int) fmax(2.f,  // force 2 at the beginning of the game
                             //
                       day_count * 2.f * customer_spawn_multiplier);
        const float time_between = round_length / new_total;

        log_info("Updating progression, setting new spawn total to {}",
                 new_total);
        entity
            .get<IsSpawner>()  //
            .set_total(new_total)
            .set_time_between(time_between);
        return;
    }
}

void reduce_impatient_customers(Entity& entity, float dt) {
    if (entity.is_missing<HasPatience>()) return;
    HasPatience& hp = entity.get<HasPatience>();

    if (!hp.should_pass_time()) return;

    hp.pass_time(dt);

    if (hp.pct() <= 0) hp.reset();
}

void pop_out_when_colliding(Entity& entity, float) {
    const auto no_clip_on =
        GLOBALS.get_or_default<bool>("no_clip_enabled", false);
    if (no_clip_on) return;

    // Only popping out players right now
    if (!check_type(entity, EntityType::Player)) return;

    OptEntity match = EntityHelper::getOverlappingEntityIfExists(
        entity, 0.75f,
        [](const Entity& entity) { return entity.has<IsSolid>(); });
    if (!match) return;

    const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
    if (chf.is_holding_furniture()) return;
    if (chf.furniture_id() == match->id) return;

    const Rectangle playerBounds = entity.get<Transform>().rectangular_bounds();
    const Rectangle matchBounds = match->get<Transform>().rectangular_bounds();

    vec2 new_position = entity.get<Transform>().as2();

    float dx = playerBounds.x + (playerBounds.width / 2) - matchBounds.x -
               (matchBounds.width / 2);
    float dy = playerBounds.y + (playerBounds.height / 2) - matchBounds.y -
               (matchBounds.height / 2);

    if (dx != 0) {
        new_position.x += util::sgn(dx) * matchBounds.width * 0.75f;
    }
    if (dy != 0) {
        new_position.y += util::sgn(dy) * matchBounds.height * 0.75f;
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

        amount_in_cart +=
            std::max(0, get_price_for_entity_type(
                            marked_entity->get<DebugName>().get_type()));
    }

    OptEntity sophie = EntityHelper::getFirstOfType(EntityType::Sophie);
    if (sophie.valid()) {
        IsBank& bank = sophie->get<IsBank>();
        bank.update_cart(amount_in_cart);
    }

    // Hack to force the validation function to run every frame
    OptEntity purchase_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsTriggerArea>()) return false;
            const IsTriggerArea& ita = entity.get<IsTriggerArea>();
            return ita.type == IsTriggerArea::Type::Store_BackToPlanning;
        });
    if (purchase_area.valid()) {
        (void) purchase_area->get<IsTriggerArea>().should_progress();
    }
}

void cleanup_old_store_options() {
    // TODO :STORE_CLEANUP: instead we should just keep track of the store
    // spawned ones and delete them floormarked
    float rad = 20;
    const auto ents = EntityHelper::getAllInRange(
        {STORE_ORIGIN - rad, -1.f * rad}, {STORE_ORIGIN + rad, rad});

    for (Entity& entity : ents) {
        // Skip the permananent ones
        if (entity.has<IsFloorMarker>()) continue;
        if (entity.has<IsTriggerArea>()) continue;

        entity.cleanup = true;
    }
}

void generate_store_options() {
    // Figure out what kinds of things we can spawn generally
    // - what is spawnable?
    // - are they capped by progression? (alcohol / fruits for sure right?)
    // choose a couple options to spawn
    // - how many?
    // spawn them
    // - use the place machine thing

    OptEntity spawn_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            return fm.type == IsFloorMarker::Type::Store_SpawnArea;
        });

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsProgressionManager& ipp = sophie.get<IsProgressionManager>();
    const EntityTypeSet& unlocked = ipp.enabled_entity_types();
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    int num_to_spawn =
        irsm.get<int>(IsRoundSettingsManager::Config::Key::NumStoreSpawns);

    while (num_to_spawn) {
        int entity_type_id = bitset_utils::get_random_enabled_bit(unlocked);
        EntityType etype = magic_enum::enum_value<EntityType>(entity_type_id);
        if (get_price_for_entity_type(etype) <= 0) continue;

        log_info("generate_store_options: random: {}",
                 magic_enum::enum_name<EntityType>(etype));

        auto& entity = EntityHelper::createEntity();
        bool success = convert_to_type(
            etype, entity,
            spawn_area->get<Transform>().as2() + vec2{
                                                     1.f * randIn(-3, 3),
                                                     1.f * randIn(-3, 3),
                                                 });
        if (success) {
            num_to_spawn--;
        } else {
            entity.cleanup = true;
        }
    }
}

void move_purchased_furniture() {
    // Grab the overlap area so we can see what it marked
    OptEntity purchase_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            return fm.type == IsFloorMarker::Type::Store_PurchaseArea;
        });
    const IsFloorMarker& ifm = purchase_area->get<IsFloorMarker>();

    // Grab the plannig spawn area so we can place in the right spot
    OptEntity spawn_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            return fm.type == IsFloorMarker::Type::Planning_SpawnArea;
        });
    vec3 spawn_position = spawn_area->get<Transform>().pos();

    OptEntity sophie = EntityHelper::getFirstOfType(EntityType::Sophie);
    VALIDATE(sophie.valid(), "sophie should exist when moving furniture");
    IsBank& bank = sophie->get<IsBank>();

    int amount_in_cart = 0;

    // for every marked, move them over
    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;
        Transform& transform = marked_entity->get<Transform>();
        transform.update(spawn_position);
        transform.update_y(0);

        // Some items can hold other items, we should move that item too
        // they arent being caught by the marker since we only mark solid items
        system_manager::update_held_item_position(marked_entity.asE(), 0.f);

        // Its free!
        if (marked_entity->has<IsFreeInStore>()) continue;

        amount_in_cart +=
            std::max(0, get_price_for_entity_type(
                            marked_entity->get<DebugName>().get_type()));
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

    entities.insert(entities.end(), ents.begin(), ents.end());
    entities.insert(entities.end(), players.begin(), players.end());

    oldAll = entities;

    // TODO should we not do any updates for client?
    // any changes will get overwritten by server every frame
    // but maybe itll matter
    // if (!is_server()) return;

    // actual update
    {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());

        if (GameState::get().is_lobby_like()) {
            //
        } else if (GameState::get().is(game::State::Store)) {
            store_update(entities, dt);
        } else if (GameState::get().is(game::State::ModelTest)) {
            model_test_update(entities, dt);
        } else if (GameState::get().is(game::State::Progression)) {
            progression_update(entities, dt);
        } else if (GameState::get().is_game_like()) {
            if (GameState::get().in_round()) {
                in_round_update(entities, dt);
            } else if (GameState::get().in_planning()) {
                planning_update(entities, dt);
            }
            game_like_update(entities, dt);
        }
        always_update(entities, dt);
        process_state_change(entities, dt);
    }
}

void SystemManager::update_local_players(const Entities& players, float dt) {
    for (const auto& entity : players) {
        // TODO fix this if we have more than one local player
        firstPlayerID = entity->id;
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
    const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
    if (transitions.empty()) return;

    const auto onRoundFinished = [&]() {
        for_each(entities, dt, [](Entity& entity, float dt) {
            system_manager::delete_floating_items_when_leaving_inround(entity);
            system_manager::delete_held_items_when_leaving_inround(entity);
            system_manager::delete_customers_when_leaving_inround(entity);
            system_manager::reset_customer_spawner_when_leaving_inround(entity);
            system_manager::reset_max_gen_when_after_deletion(entity);
            system_manager::reset_toilet_when_leaving_inround(entity);
            system_manager::increment_day_count(entity, dt);

            // Handle updating all the things that rely on progression
            system_manager::update_new_max_customers(entity, dt);

            // I think this will only happen when you debug change round while
            // customers are already in line, but doesnt hurt to reset
            system_manager::reset_register_queue_when_leaving_inround(entity);
        });
    };

    const auto onRoundStarted = [&]() {
        for_each(entities, dt, [](Entity& entity, float) {
            system_manager::handle_autodrop_furniture_when_exiting_planning(
                entity);
            system_manager::release_mop_buddy_at_start_of_day(entity);
            system_manager::delete_trash_when_leaving_planning(entity);
        });
    };

    const auto onStoreEntered = [&]() {
        system_manager::store::generate_store_options();
    };

    const auto onStoreLeave = [&]() {
        system_manager::store::move_purchased_furniture();
        system_manager::store::cleanup_old_store_options();
    };

    for (const auto& transition : transitions) {
        const auto [old_state, new_state] = transition;
        // We just always ignore paused transitions since
        // the game state didnt actually change in a meaningful way
        if (old_state == game::State::Paused) continue;
        if (new_state == game::State::Paused) continue;

        if (old_state == game::State::InRound) {
            onRoundFinished();
        }
        if (new_state == game::State::InRound) {
            onRoundStarted();
        }
        if (new_state == game::State::Store) {
            onStoreEntered();
        }
        if (old_state == game::State::Store) {
            onStoreLeave();
        }
    }

    transitions.clear();
}

void SystemManager::always_update(const Entities& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        system_manager::clear_all_floor_markers(entity, dt);
        system_manager::mark_item_in_floor_area(entity, dt);
        system_manager::reset_highlighted(entity, dt);

        // TODO should be just planning + lobby?
        // maybe a second one for highlighting items?
        system_manager::highlight_facing_furniture(entity, dt);
        system_manager::transform_snapper(entity, dt);
        system_manager::update_held_item_position(entity, dt);

        system_manager::process_trigger_area(entity, dt);

        system_manager::render_manager::update_character_model_from_index(
            entity, dt);

        // TODO :SPEED: originally this was running in "process_game_state"
        // and only supposed to run on transitions but
        // when i fixed it to actually run only on transitions
        // it broke the model for vodka (just different one) and lime
        // (invisible)
        //
        // For now its okay to stay here its just a perf thing
        system_manager::refetch_dynamic_model_names(entity, dt);
    });
}

void SystemManager::game_like_update(const Entities& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        system_manager::run_timer(entity, dt);
        system_manager::process_pnumatic_pipe_pairing(entity, dt);

        system_manager::process_is_container_and_should_backfill_item(entity,
                                                                      dt);

        // this function also handles the map validation code
        // rename it
        system_manager::update_sophie(entity, dt);
    });
}

void SystemManager::model_test_update(
    const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        // system_manager::process_grabber_items(entity, dt);
        // system_manager::process_conveyer_items(entity, dt);
        // system_manager::process_grabber_filter(entity, dt);
        // system_manager::process_pnumatic_pipe_movement(entity, dt);

        // should move all the container functions into its own
        // function?
        system_manager::process_is_container_and_should_update_item(entity, dt);
        // This one should be after the other container ones
        system_manager::process_is_indexed_container_holding_incorrect_item(
            entity, dt);

        system_manager::process_is_container_and_should_backfill_item(entity,
                                                                      dt);
    });
}

void SystemManager::in_round_update(
    const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        system_manager::reset_customers_that_need_resetting(entity);
        //
        system_manager::job_system::in_round_update(entity, dt);
        system_manager::process_grabber_items(entity, dt);
        system_manager::process_conveyer_items(entity, dt);
        system_manager::process_grabber_filter(entity, dt);
        system_manager::process_pnumatic_pipe_movement(entity, dt);
        // should move all the container functions into its own
        // function?
        system_manager::process_is_container_and_should_update_item(entity, dt);
        // This one should be after the other container ones
        system_manager::process_is_indexed_container_holding_incorrect_item(
            entity, dt);

        system_manager::process_has_rope(entity, dt);
        system_manager::process_spawner(entity, dt);
        system_manager::process_squirter(entity, dt);
        system_manager::process_trash(entity, dt);
        system_manager::reset_empty_work_furniture(entity, dt);
        system_manager::reduce_impatient_customers(entity, dt);
    });
}

void SystemManager::store_update(const Entities& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        // If you add something here think should it also go in planning?
        system_manager::update_held_furniture_position(entity, dt);
        system_manager::store::cart_management(entity, dt);
        system_manager::pop_out_when_colliding(entity, dt);

        // game like
        system_manager::process_is_container_and_should_backfill_item(entity,
                                                                      dt);
    });
}

void SystemManager::planning_update(
    const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        // If you add something here think should it also go in store?
        system_manager::update_held_furniture_position(entity, dt);
        system_manager::pop_out_when_colliding(entity, dt);
    });
}

void SystemManager::progression_update(const Entities& entity_list, float dt) {
    for_each(entity_list, dt, [](Entity& entity, float dt) {
        // TODO this runs every progression frame when it probably just needs to
        // run on transition
        system_manager::progression::collect_upgrade_options(entity, dt);
    });
}

void SystemManager::render_entities(const Entities& entities, float dt) const {
    const bool debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);

    // debug only
    system_manager::render_manager::render_walkable_spots(dt);

    // TODO do measurements on if the game actually runs faster with camera
    // culling
    //
    // GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);

    for_each(entities, dt, [debug_mode_on](const Entity& entity, float dt) {
        // vec2 e_pos = entity.get<Transform>().as2();
        // if (vec::distance(e_pos, vec::to2(cam.camera.position)) > 50.f) {
        // return;
        // }

        system_manager::render_manager::render(entity, dt, debug_mode_on);
    });
}

void SystemManager::render_ui(const Entities& entities, float dt) const {
    // const auto debug_mode_on =
    // GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    system_manager::ui::render_normal(entities, dt);
}
