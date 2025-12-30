#include "can_hold_item.h"

#include "../entity_helper.h"

Entity& CanHoldItem::item() const {
    OptEntity opt = EntityHelper::resolve(held_item_handle);
    if (!opt) {
        log_error("CanHoldItem::item() - held_item_handle is invalid or stale");
    }
    return opt.asE();
}

const Entity& CanHoldItem::const_item() const {
    OptEntity opt = EntityHelper::resolve(held_item_handle);
    if (!opt) {
        log_error(
            "CanHoldItem::const_item() - held_item_handle is invalid or stale");
    }
    return opt.asE();
}
