#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_store_spawned.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../../entity_query.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct CleanUpOldStoreOptionsSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }
    virtual void once(float) override {
        OptEntity cart_area =
            EQ().whereHasComponent<IsFloorMarker>()
                .whereLambda([](const Entity& entity) {
                    if (entity.is_missing<IsFloorMarker>()) return false;
                    const IsFloorMarker& fm = entity.get<IsFloorMarker>();
                    return fm.type == IsFloorMarker::Type::Store_PurchaseArea;
                })
                .include_store_entities()
                .gen_first();

        OptEntity locked_area =
            EQ().whereHasComponent<IsFloorMarker>()
                .whereLambda([](const Entity& entity) {
                    if (entity.is_missing<IsFloorMarker>()) return false;
                    const IsFloorMarker& fm = entity.get<IsFloorMarker>();
                    return fm.type == IsFloorMarker::Type::Store_LockedArea;
                })
                .include_store_entities()
                .gen_first();

        for (Entity& entity : EQ().whereHasComponent<IsStoreSpawned>()
                                  .include_store_entities()
                                  .gen()) {
            // ignore antyhing in the cart
            if (cart_area) {
                if (cart_area->get<IsFloorMarker>().is_marked(entity.id)) {
                    continue;
                }
            }

            // ignore anything locked
            if (locked_area) {
                if (locked_area->get<IsFloorMarker>().is_marked(entity.id)) {
                    continue;
                }
            }

            entity.cleanup = true;

            // Also cleanup the item its holding if it has one
            if (entity.is_missing<CanHoldItem>()) continue;
            CanHoldItem& chi = entity.get<CanHoldItem>();
            if (!chi.is_holding_item()) continue;
            OptEntity held_opt = chi.item();
            if (held_opt) {
                held_opt.asE().cleanup = true;
            } else {
                chi.update(nullptr, entity.id);
            }
        }
    }
};

}  // namespace system_manager