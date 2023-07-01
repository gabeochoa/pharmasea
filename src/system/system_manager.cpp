

#include "system_manager.h"

#include "../camera.h"  /// probably needed by map and not included in there
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
#include "../map.h"

namespace system_manager {
void transform_snapper(std::shared_ptr<Entity> entity, float) {
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

void update_held_item_position(std::shared_ptr<Entity> entity, float) {
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

void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.update(false);
}

void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<CanHighlightOthers>()) return;
    // TODO add a player reach component
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, cho.reach(),
        [](auto e) { return e->template has<CanBeHighlighted>(); });
    if (!match) return;

    match->get<CanBeHighlighted>().update(true);
}

// TODO We need like a temporary storage for this
void move_entity_based_on_push_force(std::shared_ptr<Entity> entity, float,
                                     vec3& new_pos_x, vec3& new_pos_z) {
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

void process_grabber_items(std::shared_ptr<Entity> entity, float) {
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
void backfill_empty_container(std::shared_ptr<Entity> entity) {
    if (entity->is_missing<IsItemContainer<I>>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    auto newItem = std::make_shared<I>(entity->get<Transform>().pos(),
                                       Color({255, 15, 240, 255}));
    canHold.update(newItem, Item::HeldBy::FURNITURE);
    ItemHelper::addItem(canHold.item());
}

void process_is_container_and_should_backfill_item(
    std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.is_holding_item()) return;
    backfill_empty_container<Bag>(entity);
    backfill_empty_container<PillBottle>(entity);
}

void handle_autodrop_furniture_when_exiting_planning(
    const std::shared_ptr<Entity>& entity) {
    if (entity->is_missing<CanHoldFurniture>()) return;

    CanHoldFurniture& ourCHF = entity->get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // TODO need to find a spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

void delete_held_items_when_leaving_inround(
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

void refetch_dynamic_model_names(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<ModelRenderer>()) return;
    if (entity->is_missing<HasDynamicModelName>()) return;

    HasDynamicModelName& hDMN = entity->get<HasDynamicModelName>();
    ModelRenderer& renderer = entity->get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch());
}

void count_trigger_area_entrants(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<IsTriggerArea>()) return;

    // TODO this also isnt visible during debug mode so we should probalby do
    // the bottom
    // TODO this doesnt change every tick, store it somewhere?
    vec3 trigger_size = {
        entity->get<Transform>().size().x,
        entity->get<Transform>().size().y + TILESIZE,
        entity->get<Transform>().size().z,
    };
    auto trigger_bounds =
        get_bounds(entity->get<Transform>().pos(), trigger_size);

    int count = 0;
    for (auto& e : SystemManager::get().oldAll) {
        // TODO need a better way to match player
        if (e->get<DebugName>().name() != "player") continue;
        if (CheckCollisionBoxes(e->get<Transform>().bounds(), trigger_bounds)) {
            count++;
        }
    }
    entity->get<IsTriggerArea>().update_entrants(count);
}

void update_trigger_area_percent(const std::shared_ptr<Entity>& entity,
                                 float dt) {
    if (entity->is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity->get<IsTriggerArea>();
    if (ita.should_wave()) {
        ita.increase_progress(dt);
    } else {
        ita.decrease_progress(dt);
    }
}

void process_trigger_area(const std::shared_ptr<Entity>& entity, float dt) {
    count_trigger_area_entrants(entity, dt);
    update_trigger_area_percent(entity, dt);
}

}  // namespace system_manager
