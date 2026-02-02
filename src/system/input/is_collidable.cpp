#include "is_collidable.h"

#include "../../components/can_be_ghost_player.h"
#include "../../components/can_be_held.h"
#include "../../components/can_hold_item.h"
#include "../../components/is_item.h"
#include "../../components/is_solid.h"

namespace system_manager {

namespace input_process_manager {

// return true if the item has collision and is currently collidable
bool is_collidable(const Entity& entity, OptEntity other) {
    // by default we disable collisions when you are holding something
    // since its generally inside your bounding box
    if (entity.has<CanBeHeld>() && entity.get<CanBeHeld>().is_set()) {
        return false;
    }

    if (entity.has<CanBeHeld_HT>() && entity.get<CanBeHeld_HT>().is_set()) {
        return false;
    }

    if (check_type(entity, EntityType::MopBuddy)) {
        if (other && check_type(other.asE(), EntityType::MopBuddyHolder)) {
            return false;
        }
    }

    if (
        // checking for person update
        other &&
        // Entity is item and held by player
        entity.has<IsItem>() &&
        entity.get<IsItem>().is_held_by(EntityType::Player) &&
        // Entity is rope
        check_type(entity, EntityType::SodaSpout) &&
        // we are a player that is holding rope
        other->has<CanHoldItem>() &&
        other->get<CanHoldItem>().is_holding_item()) {
        OptEntity held = other->get<CanHoldItem>().item();
        if (held && check_type(held.asE(), EntityType::SodaSpout)) {
            return false;
        }
    }

    if (entity.has<IsSolid>()) {
        return true;
    }

    // TODO :BE: rename this since it no longer makes sense
    // if you are a ghost player
    // then you are collidable
    if (entity.has<CanBeGhostPlayer>()) {
        return true;
    }
    return false;
}

bool is_collidable(const Entity& entity, const Entity& other) {
    // The logic is const but OptEntity doesnt support it yet
    return is_collidable(entity, OptEntity{const_cast<Entity&>(other)});
}

}  // namespace input_process_manager

}  // namespace system_manager