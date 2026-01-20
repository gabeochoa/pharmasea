#pragma once

#include "../../../ah.h"
#include "../../../components/can_order_drink.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/has_patience.h"
#include "../../../components/is_progression_manager.h"
#include "../../../components/is_round_settings_manager.h"
#include "../../../dataclass/settings.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"
#include "../../helpers/ingredient_helper.h"

namespace system_manager {

struct ResetCustomersThatNeedResettingSystem
    : public afterhours::System<CanOrderDrink> {
    IsRoundSettingsManager* irsm;
    IsProgressionManager* progressionManager;

    virtual ~ResetCustomersThatNeedResettingSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void once(float) override {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        progressionManager = &sophie.get<IsProgressionManager>();
    }

    virtual void for_each_with(Entity& entity, CanOrderDrink& cod,
                               float) override {
        if (cod.order_state != CanOrderDrink::OrderState::NeedsReset) return;

        {
            int max_num_orders =
                // max() here to avoid a situation where we get 0 after an
                // upgrade
                (int) fmax(1, irsm->get<int>(ConfigKey::MaxNumOrders));

            cod.reset_customer(max_num_orders,
                               progressionManager->get_random_unlocked_drink());
        }

        {
            // Set the patience based on how many ingredients there are
            // TODO add a map of ingredient to how long it probably takes to
            // make

            auto ingredients = get_req_ingredients_for_drink(cod.get_order());
            float patience_multiplier =
                irsm->get<float>(ConfigKey::PatienceMultiplier);
            entity.get<HasPatience>().update_max(ingredients.count() * 30.f *
                                                 patience_multiplier);
            entity.get<HasPatience>().reset();
        }
    }
};

}  // namespace system_manager