#pragma once

#include "../../ah.h"
#include "../../components/can_hold_furniture.h"
#include "../../components/can_hold_item.h"
#include "../../components/custom_item_position.h"
#include "../../entities/entity_helper.h"
#include "afterhours_held_position_helpers.h"

namespace system_manager {

// Traits for different "holder" component types
// Each specialization defines how to get the held entity and its position

template <typename HolderComponent>
struct HeldPositionTraits;

// Specialization for CanHoldItem
template <>
struct HeldPositionTraits<CanHoldItem> {
    static OptEntity get_held_entity(CanHoldItem& holder) {
        return holder.item();
    }

    static void clear_held(CanHoldItem& holder, int entity_id) {
        holder.update(nullptr, entity_id);
    }

    static vec3 get_position(Entity& entity) {
        return entity.has<CustomHeldItemPosition>()
                   ? get_new_held_position_custom(entity)
                   : get_new_held_position_default(entity);
    }
};

// Specialization for CanHoldHandTruck (which is CanHoldEntityBase<HandTruckHoldTag>)
template <>
struct HeldPositionTraits<CanHoldHandTruck> {
    static OptEntity get_held_entity(CanHoldHandTruck& holder) {
        return EntityHelper::getEntityForID(holder.held_id());
    }

    static void clear_held(CanHoldHandTruck& /*holder*/, int /*entity_id*/) {
        // CanHoldHandTruck doesn't have a clear mechanism in original code
        // The original code didn't handle null case for hand truck
    }

    static vec3 get_position(Entity& entity) {
        return entity.get<Transform>().pos();
    }
};

}  // namespace system_manager
