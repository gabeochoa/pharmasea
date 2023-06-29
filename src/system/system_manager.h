
#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/is_snappable.h"
#include "../components/responds_to_user_input.h"
#include "../components/transform.h"
#include "../engine/tracy.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "input_process_manager.h"
#include "job_system.h"
#include "rendering_system.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    transform.update(entity->has<IsSnappable>() ? transform.snap_position()
                                                : transform.raw());
}

// TODO if cannot be placed in this spot make it obvious to the user
void update_held_furniture_position(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing_any<Transform, CanHoldFurniture>()) return;

    const Transform& transform = entity->get<Transform>();

    CanHoldFurniture& can_hold_furniture = entity->get<CanHoldFurniture>();
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

    can_hold_furniture.furniture()->get<Transform>().update(new_pos);
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;

    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    const Transform& transform = entity->get<Transform>();

    vec3 new_pos = transform.pos();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        switch (custom_item_position.positioner) {
            case CustomHeldItemPosition::Positioner::Default:
                new_pos.y += TILESIZE / 4;
                break;
            case CustomHeldItemPosition::Positioner::Table:
                new_pos.y += TILESIZE / 2;
                break;
            case CustomHeldItemPosition::Positioner::Conveyer:
                if (entity->is_missing<ConveysHeldItem>()) {
                    log_warn(
                        "A conveyer positioned item needs ConveysHeldItem");
                    break;
                }
                ConveysHeldItem& conveysHeldItem =
                    entity->get<ConveysHeldItem>();
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
                break;
        }
        can_hold_item.item()->update_position(new_pos);
        return;
    }

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
    can_hold_item.item()->update_position(new_pos);
}

inline void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.update(false);
}

inline void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    Transform& transform = entity->get<Transform>();

    if (entity->is_missing<CanHighlightOthers>()) return;
    // TODO add a player reach component
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, cho.reach(), [](auto&&) { return true; });
    if (!match) return;
    if (!match->has<CanBeHighlighted>()) return;
    match->get<CanBeHighlighted>().update(true);
}

// TODO We need like a temporary storage for this
inline void move_entity_based_on_push_force(std::shared_ptr<Entity> entity,
                                            float, vec3& new_pos_x,
                                            vec3& new_pos_z) {
    CanBePushed& cbp = entity->get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force().x;
    cbp.update_x(0.0f);

    new_pos_z.z += cbp.pushed_force().z;
    cbp.update_z(0.0f);
}

void process_conveyer_items(std::shared_ptr<Entity> entity, float dt) {
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<CanHoldItem>()) return;
    if (entity->is_missing<ConveysHeldItem>()) return;
    if (entity->is_missing<CanBeTakenFrom>()) return;

    CanHoldItem& canHold = entity->get<CanHoldItem>();
    CanBeTakenFrom& canBeTakenFrom = entity->get<CanBeTakenFrom>();
    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

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

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, 1.f, [entity](std::shared_ptr<Furniture> furn) {
            // cant be us
            if (entity->id == furn->id) return false;
            // needs to be able to hold something
            if (furn->is_missing<CanHoldItem>()) return false;
            // has to be empty
            return furn->get<CanHoldItem>().empty();
        });

    // no match means we can't continue, stay in the middle
    if (!match) {
        conveysHeldItem.relative_item_pos = 0.f;
        canBeTakenFrom.update(true);
        return;
    }

    // we got something that will take from us,
    // but only once we get close enough

    // so keep moving forward
    if (conveysHeldItem.relative_item_pos <= ConveysHeldItem::ITEM_END) {
        conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
        return;
    }

    // we reached the end, pass ownership

    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    matchCHI.update(ourCHI.item(), Item::HeldBy::FURNITURE);

    ourCHI.update(nullptr);

    canBeTakenFrom.update(true);  // we are ready to have someone grab from us
    // reset so that the next item we get starts from beginning
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

    // TODO if we are pushing onto a conveyer, we need to make sure
    // we are keeping track of the orientations
    //
    //  --> --> in this case we want to place at 0.f
    //
    //          ^
    //    -->-> |     in this we want to place at 0.f instead of -0.5
}

inline void process_grabber_items(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) {
        log_warn("process grabber missing transform {}", entity->id);
        log_warn("process grabber missing transform {}",
                 entity->get<DebugName>().name());
    }

    Transform& transform = entity->get<Transform>();

    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    // we are already holding something so
    if (canHold.is_holding_item()) return;

    // Should only run this for conveyers
    if (entity->is_missing<ConveysHeldItem>()) return;
    // Should only run for grabbers
    if (entity->is_missing<CanGrabFromOtherFurniture>()) return;

    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

    auto behind =
        transform.offsetFaceDirection(transform.face_direction(), 180);
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        transform.as2(), 1.f, behind,
        [entity](std::shared_ptr<Furniture> furn) {
            // cant be us
            if (entity->id == furn->id) return false;
            // needs to be able to hold something
            if (furn->is_missing<CanHoldItem>()) return false;
            // doesnt have anything
            if (furn->get<CanHoldItem>().empty()) return false;
            //
            if (furn->is_missing<CanBeTakenFrom>()) return false;
            return furn->get<CanBeTakenFrom>().can_take_from();
        });

    // No furniture behind us
    if (!match) return;

    // Grab from the furniture match
    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    ourCHI.update(matchCHI.item(), Item::HeldBy::FURNITURE);
    matchCHI.update(nullptr);

    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
}

template<typename I>
inline void backfill_empty_container(std::shared_ptr<Entity> entity) {
    if (entity->is_missing<IsItemContainer<I>>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    auto newItem = std::make_shared<I>(entity->get<Transform>().pos(),
                                       Color({255, 15, 240, 255}));
    canHold.update(newItem, Item::HeldBy::FURNITURE);
    ItemHelper::addItem(canHold.item());
}

inline void process_is_container_and_should_backfill_item(
    std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.is_holding_item()) return;
    backfill_empty_container<Bag>(entity);
    backfill_empty_container<PillBottle>(entity);
}

inline void handle_autodrop_furniture_when_exiting_planning(
    const std::shared_ptr<Entity>& entity) {
    if (entity->is_missing<CanHoldFurniture>()) return;

    CanHoldFurniture& ourCHF = entity->get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // TODO need to find a spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

inline void delete_held_items_when_leaving_inround(
    const std::shared_ptr<Entity>& entity) {
    // TODO this doesnt seem to work
    // you keep holding it even after the transition

    log_warn("delete_held_items_when_leaving_inround");
    if (entity->is_missing<CanHoldItem>()) return;
    log_warn("delete_ canhold {} {}", entity->get<DebugName>().name(),
             entity->id);

    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.empty()) return;
    log_warn("delete_ notempty");

    // Mark it as deletable
    std::shared_ptr<Item>& item = canHold.item();
    item->cleanup = true;

    // let go of the item
    canHold.update(nullptr);

    log_error("deleting item :");
}

inline void refetch_dynamic_model_names(const std::shared_ptr<Entity>& entity,
                                        float) {
    if (entity->is_missing<ModelRenderer>()) return;
    if (entity->is_missing<HasDynamicModelName>()) return;

    HasDynamicModelName& hDMN = entity->get<HasDynamicModelName>();
    ModelRenderer& renderer = entity->get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch());
}

}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

    bool state_transitioned_round_to_planning = false;
    bool state_transitioned_planning_to_round = false;

    void on_game_state_change(game::State new_state, game::State old_state) {
        // log_warn("system manager on gamestate change from {} to {}",
        // old_state, new_state);

        if (old_state == game::State::InRound &&
            new_state == game::State::Planning) {
            state_transitioned_round_to_planning = true;
        }

        if (old_state == game::State::Planning &&
            new_state == game::State::InRound) {
            state_transitioned_planning_to_round = true;
        }
    }

    void update(const Entities& entities, float dt) {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());
        // TODO do we run game updates during paused?

        if (GameState::get().is(game::State::InRound)) {
            in_round_update(entities, dt);
        } else {
            planning_update(entities, dt);
        }

        always_update(entities, dt);
        process_state_change(entities, dt);
    }

    void update_all_entities(const Entities& players, float dt) {
        // TODO speed?
        Entities all;
        Entities ents = EntityHelper::get_entities();

        all.reserve(players.size() + ents.size());

        all.insert(all.end(), players.begin(), players.end());
        all.insert(all.end(), ents.begin(), ents.end());

        update(all, dt);
    }

    void process_inputs(const Entities& entities, const UserInputs& inputs) {
        for (auto& entity : entities) {
            if (entity->is_missing<RespondsToUserInput>()) continue;
            for (auto input : inputs) {
                system_manager::input_process_manager::process_input(entity,
                                                                     input);
            }
        }
    }

    void render_entities(const Entities& entities, float dt) const {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            render_debug(entities, dt);
            render_normal(entities, dt);
        } else {
            render_normal(entities, dt);
        }
    }

    void render_items(Items items, float) const {
        for (auto i : items) {
            if (i) i->render();
            if (!i) log_warn("we have invalid items");
        }
    }

   private:
    void process_state_change(
        const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
        if (state_transitioned_round_to_planning) {
            state_transitioned_round_to_planning = false;
            for (auto& entity : entities) {
                system_manager::delete_held_items_when_leaving_inround(entity);
            }
        }

        if (state_transitioned_planning_to_round) {
            state_transitioned_planning_to_round = false;
            for (auto& entity : entities) {
                system_manager::handle_autodrop_furniture_when_exiting_planning(
                    entity);
            }
        }

        // All transitions
        for (auto& entity : entities) {
            system_manager::refetch_dynamic_model_names(entity, dt);
        }
    }

    void always_update(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) {
        for (auto& entity : entity_list) {
            system_manager::reset_highlighted(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::input_process_manager::collect_user_input(entity,
                                                                      dt);
            system_manager::update_held_item_position(entity, dt);

            // TODO this is in the render manager but its not really a
            // render thing but at the same time it kinda is idk This could
            // run only in lobby if we wanted to distinguish
            system_manager::render_manager::update_character_model_from_index(
                entity, dt);
        }
    }

    void in_round_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::job_system::in_round_update(entity, dt);
            system_manager::process_grabber_items(entity, dt);
            system_manager::process_conveyer_items(entity, dt);
            system_manager::process_is_container_and_should_backfill_item(
                entity, dt);
        }
    }

    void planning_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::highlight_facing_furniture(entity, dt);
            system_manager::update_held_furniture_position(entity, dt);
        }
    }

    void render_normal(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_normal(entity, dt);
            system_manager::render_manager::render_floating_name(entity, dt);
            system_manager::render_manager::render_progress_bar(entity, dt);
        }
    }

    void render_debug(const std::vector<std::shared_ptr<Entity>>& entity_list,
                      float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_debug(entity, dt);
        }
    }
};
