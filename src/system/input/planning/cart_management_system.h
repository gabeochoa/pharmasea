#pragma once

#include "../../../ah.h"
#include "../../../building_locations.h"
#include "../../../components/is_bank.h"
#include "../../../components/is_floor_marker.h"
#include "../../../components/is_free_in_store.h"
#include "../../../components/is_store_spawned.h"
#include "../../../entities/entity_helper.h"
#include "../../../entities/entity_type.h"
#include "../../core/system_manager.h"
#include "../../system_utilities.h"

namespace system_manager {

namespace planning {

struct CartManagementSystem
    : public afterhours::System<
          IsFloorMarker, afterhours::tags::All<EntityType::FloorMarker>> {
    OptEntity sophie;

    virtual bool should_run(const float) override {
        if (!system_utils::should_run_planning_system()) return false;
        sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        return true;
    }

    virtual void for_each_with(Entity&, IsFloorMarker& ifm, float) override {
        if (ifm.type != IsFloorMarker::Store_PurchaseArea) return;

        int amount_in_cart = 0;

        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;

            if (marked_entity->has<IsFreeInStore>()) continue;
            if (marked_entity->is_missing<IsStoreSpawned>()) continue;

            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }

        if (sophie.has_value()) {
            IsBank& bank = sophie->get<IsBank>();
            bank.update_cart(amount_in_cart);
        }

        OptEntity purchase_area =
            EQ().whereTriggerAreaOfType(
                    IsTriggerArea::Type::Store_BackToPlanning)
                .gen_first();
        if (purchase_area.valid()) {
            (void) purchase_area->get<IsTriggerArea>().should_progress();
        }
    }
};

}  // namespace planning

}  // namespace system_manager