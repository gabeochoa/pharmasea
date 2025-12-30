#include "post_deserialize_fixups.h"

#include <unordered_set>

#include "components/adds_ingredient.h"
#include "components/can_hold_item.h"
#include "components/can_pathfind.h"
#include "components/responds_to_day_night.h"
#include "engine/log.h"
#include "entity_id.h"

namespace post_deserialize_fixups {

void run(Entities& entities) {
    std::unordered_set<EntityID> ids;
    ids.reserve(entities.size());
    for (const auto& sp : entities) {
        if (!sp) continue;
        ids.insert(sp->id);
    }

    for (const auto& sp : entities) {
        if (!sp) continue;
        Entity& e = *sp;

        // These components store "parent" as an EntityID today; in practice
        // this is always the owning entity.
        if (e.has<CanPathfind>()) {
            e.get<CanPathfind>().set_parent(e.id);
        }
        if (e.has<RespondsToDayNight>()) {
            e.get<RespondsToDayNight>().set_parent(e.id);
        }
        if (e.has<AddsIngredient>()) {
            e.get<AddsIngredient>().set_parent(e.id);
        }

        // Clear broken handle references that used to be implicitly carried
        // by smart pointers during serialization.
        if (e.has<CanHoldItem>()) {
            CanHoldItem& chi = e.get<CanHoldItem>();
            if (chi.is_holding_item()) {
                const EntityID held = chi.item_id();
                if (held == entity_id::INVALID || !ids.contains(held)) {
                    log_error(
                        "post_deserialize_fixups: entity {} holds missing item "
                        "id {}; clearing",
                        e.id, held);
                    chi.update(nullptr, entity_id::INVALID);
                } else {
                    // IsItem doesn't serialize held_by today; re-apply from the
                    // holder's CanHoldItem state.
                    Entity& item = chi.item();
                    if (item.has<IsItem>()) {
                        item.get<IsItem>().set_held_by(chi.hb_type(), e.id);
                    } else {
                        log_error(
                            "post_deserialize_fixups: entity {} holds item {} "
                            "but item is missing IsItem",
                            e.id, held);
                    }
                }
            }
        }
    }
}

}  // namespace post_deserialize_fixups

