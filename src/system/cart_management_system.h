#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_bank.h"
#include "../components/is_floor_marker.h"
#include "../components/is_free_in_store.h"
#include "../components/is_store_spawned.h"
#include "../components/is_trigger_area.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../entity_type.h"

namespace system_manager {

struct CartManagementSystem
    : public afterhours::System<
          IsFloorMarker, afterhours::tags::All<EntityType::FloorMarker>> {
    OptEntity sophie;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie->get<HasDayNightTimer>();
            return hastimer.is_daytime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, IsFloorMarker& ifm, float) override {
        if (ifm.type != IsFloorMarker::Store_PurchaseArea) return;

        int amount_in_cart = 0;

        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;

            // Its free!
            if (marked_entity->has<IsFreeInStore>()) continue;
            // it was already purchased or is otherwise just randomly in the
            // store
            if (marked_entity->is_missing<IsStoreSpawned>()) continue;

            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }

        if (sophie.has_value()) {
            IsBank& bank = sophie->get<IsBank>();
            bank.update_cart(amount_in_cart);
        }

        // Hack to force the validation function to run every frame
        OptEntity purchase_area = EntityHelper::getMatchingTriggerArea(
            IsTriggerArea::Type::Store_BackToPlanning);
        if (purchase_area.valid()) {
            (void) purchase_area->get<IsTriggerArea>().should_progress();
        }
    }
};

}  // namespace system_manager
