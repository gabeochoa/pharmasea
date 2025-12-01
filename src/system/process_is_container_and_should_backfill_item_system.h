#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/indexer.h"
#include "../components/is_item_container.h"
#include "../components/is_progression_manager.h"
#include "../components/transform.h"
#include "../dataclass/ingredient.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_type.h"

namespace system_manager {

// Helper template function for backfilling containers
template<typename... TArgs>
inline void backfill_empty_container(const EntityType& match_type,
                                     Entity& entity, vec3 pos,
                                     TArgs&&... args) {
    // TODO are there any other callers that wuld fail this check?
    if (entity.is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity.get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    Entity& item =
        EntityHelper::createItem(iic.type(), pos, std::forward<TArgs>(args)...);
    // ^ cannot be const because converting to SharedPtr v

    // TODO do we need shared pointer here? (vs just id?)
    canHold.update(EntityHelper::getEntityAsSharedPtr(item), entity.id);
}

struct ProcessIsContainerAndShouldBackfillItemSystem
    : public afterhours::System<IsItemContainer, CanHoldItem> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, IsItemContainer& iic,
                               CanHoldItem& canHold, float) override {
        if (canHold.is_holding_item()) return;

        auto pos = entity.get<Transform>().pos();

        if (iic.should_use_indexer() && entity.has<Indexer>()) {
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

            backfill_empty_container(iic.type(), entity, pos, indexer.value());
            entity.get<Indexer>().mark_change_completed();
            return;
        }

        backfill_empty_container(iic.type(), entity, pos);
    }
};

}  // namespace system_manager
