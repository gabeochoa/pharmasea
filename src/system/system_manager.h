
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
    const Transform& transform = entity->get<Transform>();

    if (entity->is_missing<CanHoldFurniture>()) return;

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

    // TODO only seems to work for the host
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
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    // TODO this is impossible to read, what can we do to fix this while
    // keeping it configurable
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        // TODO add a player reach component
        transform.as2(), cho.reach(), transform.face_direction(),
        [](std::shared_ptr<Furniture>) { return true; });
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
    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    // we are not holding anything
    if (canHold.empty()) return;

    if (entity->is_missing<ConveysHeldItem>()) return;
    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->is_missing<CanBeTakenFrom>()) return;
    CanBeTakenFrom& canBeTakenFrom = entity->get<CanBeTakenFrom>();

    canBeTakenFrom.update(false);

    // if the item is less than halfway, just keep moving it along
    if (conveysHeldItem.relative_item_pos <= 0.f) {
        conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
        return;
    }

    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        transform.as2(), 1.f, transform.face_direction(),
        [entity](std::shared_ptr<Furniture> furn) {
            return entity->id != furn->id &&
                   // TODO need to merge this into the system manager one
                   // but cant yet
                   furn->get<CanHoldItem>().empty();
        });
    // no match means we cant continue, stay in the middle
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

    canBeTakenFrom.update(true);
    // we reached the end, pass ownership

    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    matchCHI.item() = ourCHI.item();
    matchCHI.item()->held_by = Item::HeldBy::FURNITURE;
    ourCHI.item() = nullptr;
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

    // TODO if we are pushing onto a conveyer, we need to make sure
    // we are keeping track of the orientations
    //
    //  --> --> in this case we want to place at 0.f
    //
    //          ^
    //    -->-> |     in this we want to place at 0.f instead of -0.5
}

void process_grabber_items(std::shared_ptr<Entity> entity, float) {
    // Should only run this for conveyers
    if (entity->is_missing<ConveysHeldItem>()) return;
    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

    // Should only run for grabbers
    if (entity->is_missing<CanGrabFromOtherFurniture>()) return;

    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    // we are already holding something so
    if (canHold.is_holding_item()) return;

    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        transform.as2(), 1.f,
        // Behind
        transform.offsetFaceDirection(transform.face_direction(), 180),
        [entity](std::shared_ptr<Furniture> furn) {
            return entity->id != furn->id &&
                   furn->get<CanHoldItem>().can_take_item_from();
        });

    // No furniture behind us
    if (!match) return;

    // Grab from the furniture match
    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    ourCHI.update(matchCHI.item());
    ourCHI.item()->held_by = Item::HeldBy::FURNITURE;
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

    // TODO nullptr?
    matchCHI.update(nullptr);
}

void process_is_container_and_should_destroy_item(
    std::shared_ptr<Entity> entity, float) {
    /*
     * If we are an item container and we are holding an instance
     * then we should just destroy it
     * */

    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();

    if (canHold.empty()) return;

    const auto item_container_destroy_matching =
        []<typename I>(std::shared_ptr<Entity> entity,
                       std::shared_ptr<I> item = nullptr) {
            if (!item) return;
            if (entity->has<IsItemContainer<I>>()) return;

            if (entity->is_missing<CanHoldItem>()) return;

            // TODO disabling the code below because it is just generating tons
            // of items per frame and causing slowdowns
            return;
            // CanHoldItem& canHold = entity->get<CanHoldItem>();
            // TODO is this the api we want
            // canHold.item().reset(
            // // TODO what is this color and what is it for
            // new I(entity->get<Transform>().pos(),
            // Color({255, 15, 240, 255})));
            // ItemHelper::addItem(canHold.item());
        };

    item_container_destroy_matching(entity,
                                    dynamic_pointer_cast<Bag>(canHold.item()));
    item_container_destroy_matching(
        entity, dynamic_pointer_cast<PillBottle>(canHold.item()));
}

// TODO fix this
// void process_register_waiting_queue(std::shared_ptr<Entity> entity, float dt)
// { if (entity->is_missing<HasWaitingQueue>()) return; HasWaitingQueue&
// hasWaitingQueue = entity->get<HasWaitingQueue>();
// }
//

}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    void update(const Entities& entities, float dt) {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());
        always_update(entities, dt);
        // TODO do we run game updates during paused?
        if (GameState::get().is(game::State::InRound)) {
            in_round_update(entities, dt);
        } else {
            planning_update(entities, dt);
        }
    }

    void update(float dt) { update(EntityHelper::get_entities(), dt); }

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
    void always_update(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) {
        for (auto& entity : entity_list) {
            system_manager::reset_highlighted(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::input_process_manager::collect_user_input(entity,
                                                                      dt);
            system_manager::update_held_item_position(entity, dt);

            // TODO this is in the render manager but its not really a render
            // thing but at the same time it kinda is idk This could run only in
            // lobby if we wanted to distinguish
            system_manager::render_manager::update_character_model_from_index(
                entity, dt);
        }
    }

    void in_round_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::job_system::handle_job_holder_pushed(entity, dt);
            system_manager::job_system::update_job_information(entity, dt);
            system_manager::process_conveyer_items(entity, dt);
            system_manager::process_grabber_items(entity, dt);
            system_manager::process_is_container_and_should_destroy_item(entity,
                                                                         dt);
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
