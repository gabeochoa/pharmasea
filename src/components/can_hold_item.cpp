#include "can_hold_item.h"

#include "../entities/entity_helper.h"

OptEntity CanHoldItem::item() const { return held_item.resolve(); }

OptEntity CanHoldItem::const_item() const { return held_item.resolve(); }
