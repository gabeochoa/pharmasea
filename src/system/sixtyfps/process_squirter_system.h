#pragma once

#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/is_drink.h"
#include "../../components/is_squirter.h"
#include "../../components/transform.h"
#include "../../entity_id.h"
#include "../../entity_makers.h"
#include "../../entity_query.h"
#include "../../vec_util.h"

namespace system_manager {

struct ProcessSquirterSystem
    : public afterhours::System<IsSquirter, CanHoldItem, Transform> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               IsSquirter& is_squirter, CanHoldItem& sqCHI,
                               Transform& transform, float dt) override {
        // If we arent holding anything, nothing to squirt into
        if (sqCHI.empty()) {
            is_squirter.reset();
            is_squirter.set_drink_id(-1);
            return;
        }

        // cant squirt into this !
        OptEntity drink_opt = sqCHI.item();
        if (!drink_opt) {
            sqCHI.update(nullptr, entity_id::INVALID);
            is_squirter.reset();
            is_squirter.set_drink_id(-1);
            return;
        }
        Entity& drink = drink_opt.asE();
        if (drink.is_missing<IsDrink>()) return;

        // so we got something, lets see if anyone around can give us
        // something to use

        auto pos = transform.as2();
        OptEntity closest_furniture =
            EntityQuery()
                .whereHasComponentAndLambda<CanHoldItem>(
                    [](const CanHoldItem& chi) {
                        if (chi.empty()) return false;
                        OptEntity item_opt = chi.const_item();
                        if (!item_opt) return false;
                        const Item& item = item_opt.asE();
                        // TODO should we instead check for <AddsIngredient>?
                        if (!check_type(item, EntityType::Alcohol))
                            return false;
                        return true;
                    })
                .whereInRange(pos, 1.25f)
                // NOTE: if you change this make sure that this always sorts the
                // same per game version
                .orderByDist(pos)
                .gen_first();

        if (!closest_furniture) {
            // Nothing anymore, probably someone took the item away
            // working reset
            return;
        }
        CanHoldItem& closest_chi = closest_furniture->get<CanHoldItem>();
        OptEntity item_opt = closest_chi.item();
        if (!item_opt) {
            closest_chi.update(nullptr, entity_id::INVALID);
            return;
        }
        Item& item = item_opt.asE();

        if (is_squirter.drink_id() == drink.id) {
            is_squirter.reset();
            return;
        }

        if (is_squirter.item_id() == -1) {
            is_squirter.update(item.id,
                               closest_furniture->get<Transform>().pos());
        }
        vec3 item_position = is_squirter.picked_up_at();

        if (item.id != is_squirter.item_id()) {
            log_warn("squirter : item changed while working was {} now {}",
                     is_squirter.item_id(), item.id);
            is_squirter.reset();
            return;
        }

        if (vec::distance(vec::to2(item_position),
                          closest_furniture->get<Transform>().as2()) > 1.f) {
            log_warn("squirter : we matched something different {} {}",
                     item_position, closest_furniture->get<Transform>().as2());
            // We matched something different
            is_squirter.reset();
            return;
        }

        bool complete = is_squirter.pass_time(dt);
        if (!complete) {
            // keep going :)
            return;
        }

        is_squirter.set_drink_id(drink.id);

        bool cleanup = items::_add_item_to_drink_NO_VALIDATION(drink, item);
        if (cleanup) {
            closest_furniture->get<CanHoldItem>().update(nullptr,
                                                         entity_id::INVALID);
        }
    }
};

}  // namespace system_manager
