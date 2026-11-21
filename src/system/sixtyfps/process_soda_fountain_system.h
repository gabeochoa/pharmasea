#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/is_drink.h"
#include "../../dataclass/ingredient.h"
#include "../../entities/entity_makers.h"

namespace system_manager {

struct ProcessSodaFountainSystem
    : public afterhours::System<
          CanHoldItem, afterhours::tags::All<EntityType::SodaFountain>> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity&, CanHoldItem& sfCHI, float) override {
        // If we arent holding anything, nothing to squirt into
        OptEntity drink_opt = sfCHI.item();
        if (!drink_opt) return;
        Entity& drink = drink_opt.asE();
        if (drink.is_missing<IsDrink>()) return;
        // Already has soda in it
        if (bitset_utils::test(drink.get<IsDrink>().ing(), Ingredient::Soda)) {
            return;
        }

        items::_add_ingredient_to_drink_NO_VALIDATION(drink, Ingredient::Soda);
    }
};

}  // namespace system_manager
