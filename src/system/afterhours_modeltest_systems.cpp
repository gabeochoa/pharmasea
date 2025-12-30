#include "system_manager.h"

// Component includes needed for the moved struct definitions
#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_subtype.h"
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
    //
    // TODO do we need shared pointer here? (vs just id?)
    canHold.update(item, entity.id);
}

struct ProcessIsContainerAndShouldBackfillItemSystem
    : public afterhours::System<IsItemContainer, CanHoldItem> {
    virtual bool should_run(const float) override {
        // Model Test should always run this
        if (GameState::get().is(game::State::ModelTest)) return true;

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

// System for processing is_container_and_should_update_item during in-round
// updates
struct ProcessIsContainerAndShouldUpdateItemSystem
    : public afterhours::System<Transform, IsItemContainer, Indexer,
                                CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldUpdateItemSystem() = default;

    virtual bool should_run(const float) override {
        // Model Test should always run this
        if (GameState::get().is(game::State::ModelTest)) return true;

        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               IsItemContainer& iic, Indexer& indexer,
                               CanHoldItem& canHold, float) override {
        if (!iic.should_use_indexer()) return;
        // user didnt change the index so we are good to wait
        if (indexer.value_same_as_last_render()) return;

        // Delete the currently held item
        if (canHold.is_holding_item()) {
            canHold.item().cleanup = true;
            canHold.update(nullptr, EntityID::INVALID);
        }

        auto pos = transform.pos();
        backfill_empty_container(iic.type(), entity, pos, indexer.value());
        indexer.mark_change_completed();
    }
};

struct ProcessIsIndexedContainerHoldingIncorrectItemSystem
    : public afterhours::System<Indexer, CanHoldItem> {
    virtual bool should_run(const float) override {
        // Model Test should always run this
        if (GameState::get().is(game::State::ModelTest)) return true;

        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, Indexer& indexer, CanHoldItem& canHold,
                               float) override {
        // This function is for when you have an indexed container and you put
        // the item back in but the index had changed. you put the item back in
        // but the index had changed.
        // in this case we need to clear the item because otherwise
        // they will both live there which will cause overlap and grab issues.

        if (canHold.empty()) return;

        int current_value = indexer.value();
        int item_value = canHold.item().get<HasSubtype>().get_type_index();

        if (current_value != item_value) {
            canHold.item().cleanup = true;
            canHold.update(nullptr, EntityID::INVALID);
        }
    }
};

}  // namespace system_manager

// Model test update system - processes entities during model test state
void SystemManager::register_modeltest_systems() {
    // should move all the container functions into its own
    // function?
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldUpdateItemSystem>());
    // This one should be after the other container ones
    // TODO before you migrate this, we need to look at the should_run logic
    // since the existing System<> uses is_bar_closed
    systems.register_update_system(
        std::make_unique<
            system_manager::
                ProcessIsIndexedContainerHoldingIncorrectItemSystem>());

    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldBackfillItemSystem>());
}
