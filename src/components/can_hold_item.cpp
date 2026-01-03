#include "can_hold_item.h"

#include "../entity_helper.h"

OptEntity CanHoldItem::item() const {
    return EntityHelper::getEntityForID(held_item_id);
}

OptEntity CanHoldItem::const_item() const {
    return EntityHelper::getEntityForID(held_item_id);
}
