#include "entity_ref.h"

#include "entity_helper.h"

void EntityRef::set(Entity& e) {
    id = e.id;
    // Handle will be invalid until the entity is merged/assigned a slot.
    const afterhours::EntityHandle h = EntityHelper::handle_for(e);
    if (h.valid()) {
        handle = h;
    } else {
        handle.reset();
    }
}

OptEntity EntityRef::resolve() const {
    if (handle.has_value()) {
        OptEntity opt = EntityHelper::resolve(*handle);
        if (opt) return opt;
    }
    // Fallback preserves legacy behavior (including temp-entity scans via
    // PharmaSea's EntityHelper::getEntityForID wrapper).
    return EntityHelper::getEntityForID(id);
}

Entity& EntityRef::resolve_enforced() const {
    OptEntity opt = resolve();
    if (!opt) {
        log_error("EntityRef::resolve_enforced failed for id={}", id);
    }
    return opt.asE();
}
