#include "can_hold_item.h"

#include "../entity_helper.h"

Entity& CanHoldItem::item() const {
    return EntityHelper::getEnforcedEntityForID(held_item_id);
}

const Entity& CanHoldItem::const_item() const {
    return EntityHelper::getEnforcedEntityForID(held_item_id);
}

