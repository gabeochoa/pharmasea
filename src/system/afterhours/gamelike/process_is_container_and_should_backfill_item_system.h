#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/indexer.h"
#include "../../../components/is_item_container.h"
#include "../../../components/is_progression_manager.h"
#include "../../../components/transform.h"
#include "../../../dataclass/ingredient.h"
#include "../../../engine/statemanager.h"
#include "../../../entity.h"
#include "../../../entity_helper.h"
#include "../../core/system_manager.h"

namespace system_manager {

// Forward declaration for backfill_empty_container function
// TODO move the impl into here once system manager is fully gone
template<typename... TArgs>
void backfill_empty_container(const EntityType& match_type, Entity& entity,
                              vec3 pos, TArgs&&... args);

struct ProcessIsContainerAndShouldBackfillItemSystem
    : public afterhours::System<IsItemContainer, CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldBackfillItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Skip during transitions to avoid creating hundreds of items
            // before transition logic completes
            return !timer.needs_to_process_change;
        } catch (...) {
            return true;
        }
    }

    virtual void for_each_with(Entity& entity, IsItemContainer& container,
                               CanHoldItem& canHold, float) override {
        if (canHold.is_holding_item()) return;
        auto pos = entity.get<Transform>().pos();

        if (container.should_use_indexer() && entity.has<Indexer>()) {
            Indexer& indexer = entity.get<Indexer>();

            // TODO :backfill-correct: This should match whats in
            // container_haswork

            // TODO For now we are okay doing this because the other indexer
            // (alcohol) always unlocks rum first which is index 0. if that
            // changes we gotta update this
            //  --> should we just add an assert here so we catch it quickly?
            if (check_type(entity, EntityType::FruitBasket)) {
                Entity& sophie =
                    EntityHelper::getNamedEntity(NamedEntity::Sophie);
                const IsProgressionManager& ipp =
                    sophie.get<IsProgressionManager>();

                indexer.increment_until_valid([&](int index) {
                    return !ipp.is_ingredient_locked(ingredient::Fruits[index]);
                });
            }

            backfill_empty_container(container.type(), entity, pos,
                                     indexer.value());
            entity.get<Indexer>().mark_change_completed();
            return;
        }

        backfill_empty_container(container.type(), entity, pos);
    }
};

}  // namespace system_manager