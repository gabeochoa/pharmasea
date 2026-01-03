#include "../ah.h"
#include "../building_locations.h"
#include "../components/bypass_automation_state.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_name.h"
#include "../components/has_progression.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_ai_controlled.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/responds_to_day_night.h"
#include "../components/transform.h"
#include "../engine/app.h"
#include "../engine/log.h"
#include "../engine/simulated_input/simulated_input.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_query.h"
#include "../globals.h"
#include "../network/server.h"
#include "magic_enum/magic_enum.hpp"
#include "store_management_helpers.h"
#include "system_manager.h"

namespace system_manager {

inline void delete_held_items_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // Mark it as deletable
    // let go of the item
    OptEntity held_opt = canHold.item();
    if (held_opt) {
        held_opt.asE().cleanup = true;
    }
    canHold.update(nullptr, entity_id::INVALID);
}

inline void reset_max_gen_when_after_deletion(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<IsItemContainer>()) return;

    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    // If something wasnt deleted, then just ignore it for now
    if (canHold.is_holding_item()) return;

    entity.get<IsItemContainer>().reset_generations();
}

inline void tell_customers_to_leave(Entity& entity) {
    if (!check_type(entity, EntityType::Customer)) return;

    // Force leaving job
    entity.get<IsAIControlled>().set_state(IsAIControlled::State::Leave);
    entity.removeComponentIfExists<CanPathfind>();
    entity.addComponent<CanPathfind>().set_parent(entity.id);
}

inline void update_new_max_customers(Entity& entity, float) {
    if (entity.is_missing<HasProgression>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    const HasDayNightTimer& hasTimer = sophie.get<HasDayNightTimer>();
    const int day_count = hasTimer.days_passed();

    if (check_type(entity, EntityType::CustomerSpawner)) {
        float customer_spawn_multiplier =
            irsm.get<float>(ConfigKey::CustomerSpawnMultiplier);
        float round_length = irsm.get<float>(ConfigKey::RoundLength);

        const int new_total =
            (int) fmax(2.f,  // force 2 at the beginning of the game
                             //
                       day_count * 2.f * customer_spawn_multiplier);

        // the div by 2 is so that everyone is spawned by half day, so
        // theres time for you to make their drinks and them to pay before
        // they are forced to leave
        const float time_between = (round_length / new_total) / 2.f;

        log_info("Updating progression, setting new spawn total to {}",
                 new_total);
        entity
            .get<IsSpawner>()  //
            .set_total(new_total)
            .set_time_between(time_between);
        return;
    }
}

struct GenerateStoreOptionsSystem : public afterhours::System<> {
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
    virtual void once(float) override { store::generate_store_options(); }
};

struct OpenStoreDoorsSystem
    : public afterhours::System<IsSolid,
                                afterhours::tags::All<EntityType::Door>> {
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
    virtual void for_each_with(Entity& entity, IsSolid&, float) override {
        if (!CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                 STORE_BUILDING.bounds))
            return;
        entity.removeComponentIfExists<IsSolid>();
    }
};

struct DeleteFloatingItemsWhenLeavingInRoundSystem
    : public afterhours::System<IsItem> {
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

    virtual void for_each_with(Entity& entity, IsItem& ii, float) override {
        // Its being held by something so we'll get it in the function below
        if (ii.is_held()) return;

        // Skip the mop buddy for now
        if (check_type(entity, EntityType::MopBuddy)) return;

        // mark it for cleanup
        entity.cleanup = true;

        // TODO these we likely no longer need to do
        if (false) {
            delete_held_items_when_leaving_inround(entity);

            // I dont think we want to do this since we arent
            // deleting anything anymore maybe there might be a
            // problem with spawning a simple syurup in the
            // store??
            reset_max_gen_when_after_deletion(entity);
        }
    }
};

struct TellCustomersToLeaveSystem
    : public afterhours::System<afterhours::tags::All<EntityType::Customer>,
                                IsAIControlled> {
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
    virtual void for_each_with(Entity& entity, IsAIControlled& ai,
                               float) override {
        ai.set_state(IsAIControlled::State::Leave);
        entity.removeComponentIfExists<CanPathfind>();
        entity.addComponent<CanPathfind>().set_parent(entity.id);
    }
};

// TODO :DESIGN: do we actually want to do this?
struct ResetToiletWhenLeavingInRoundSystem
    : public afterhours::System<IsToilet> {
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
    virtual void for_each_with(Entity&, IsToilet& istoilet, float) override {
        // TODO we want you to always have to clean >:)
        // but we need some way of having the customers
        // finishe the last job they were doing (as long as it
        // isnt ordering) and then leaving, otherwise the toilet
        // is stuck "inuse" when its really not
        istoilet.reset();
    }
};

struct ResetCustomerSpawnerWhenLeavingInRoundSystem
    : public afterhours::System<IsSpawner> {
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
    virtual void for_each_with(Entity&, IsSpawner& isspawner, float) override {
        isspawner.reset_num_spawned();
    }
};

struct UpdateNewMaxCustomersSystem
    : public afterhours::System<
          HasProgression, IsSpawner,
          afterhours::tags::All<EntityType::CustomerSpawner>> {
    IsRoundSettingsManager* irsm;
    HasDayNightTimer* hasTimer;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        hasTimer = &sophie.get<HasDayNightTimer>();
        try {
            return hasTimer->needs_to_process_change &&
                   hasTimer->is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    void once(float) override {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        hasTimer = &sophie.get<HasDayNightTimer>();
    }

    virtual void for_each_with(Entity&, HasProgression&, IsSpawner& isspawner,
                               float) override {
        const int day_count = hasTimer->days_passed();
        float customer_spawn_multiplier =
            irsm->get<float>(ConfigKey::CustomerSpawnMultiplier);
        float round_length = irsm->get<float>(ConfigKey::RoundLength);

        const int new_total =
            (int) fmax(2.f,  // force 2 at the beginning of the game
                       day_count * 2.f * customer_spawn_multiplier);

        // the div by 2 is so that everyone is spawned by half day, so
        // theres time for you to make their drinks and them to pay before
        // they are forced to leave
        const float time_between = (round_length / new_total) / 2.f;

        log_info("Updating progression, setting new spawn total to {}",
                 new_total);
        isspawner.set_total(new_total).set_time_between(time_between);
    }
};

struct OnNightEndedTriggerSystem
    : public afterhours::System<RespondsToDayNight> {
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
    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_night_ended();
    }
};

struct OnDayStartedTriggerSystem
    : public afterhours::System<RespondsToDayNight> {
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
    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_day_started();
    }
};

struct OnRoundFinishedTriggerSystem
    : public afterhours::System<IsRoundSettingsManager> {
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
    virtual void for_each_with(Entity&, IsRoundSettingsManager& irsm,
                               float) override {
        irsm.ran_for_hour = -1;
        irsm.config.this_hours_mods.clear();
    }
};

#if ENABLE_DEV_FLAGS
struct BypassInitSystem : public afterhours::System<BypassAutomationState> {
    virtual bool should_run(const float) override {
        if (!BYPASS_MENU && BYPASS_ROUNDS <= 0) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            sophie.addComponentIfMissing<BypassAutomationState>();
            return true;
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, BypassAutomationState& state,
                               float) override {
        if (state.initialized) return;
        int rounds_to_run = BYPASS_ROUNDS;
        if (rounds_to_run < 1 && BYPASS_MENU) {
            rounds_to_run = 1;
        }
        if (rounds_to_run <= 0) return;
        state.configure(rounds_to_run, EXIT_ON_BYPASS_COMPLETE, RECORD_INPUTS);
        BYPASS_MENU = state.bypass_enabled;
    }
};

struct BypassRoundTrackerSystem
    : public afterhours::System<BypassAutomationState> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        if (!BYPASS_MENU && BYPASS_ROUNDS <= 0) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            sophie.addComponentIfMissing<BypassAutomationState>();
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_closed();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, BypassAutomationState& state,
                               float) override {
        if (!state.initialized && (BYPASS_MENU || BYPASS_ROUNDS > 0)) {
            int rounds_to_run = BYPASS_ROUNDS;
            if (rounds_to_run < 1 && BYPASS_MENU) {
                rounds_to_run = 1;
            }
            state.configure(rounds_to_run, EXIT_ON_BYPASS_COMPLETE,
                            RECORD_INPUTS);
        }
        if (!state.bypass_enabled || state.completed) {
            BYPASS_MENU = false;
            return;
        }

        state.mark_round_complete();
        if (state.completed) {
            BYPASS_MENU = false;
            if (state.exit_on_complete || EXIT_ON_BYPASS_COMPLETE) {
                App::get().close();
            }
        } else {
            simulated_input::start_round_replay();
        }
    }
};
#endif

// Struct definitions from process_night_start_system.h moved here

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

    virtual void once(float) override { store::cleanup_old_store_options(); }
};

struct OnDayEndedSystem : public afterhours::System<RespondsToDayNight> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_day_ended();
    }
};

struct CloseBuildingsWhenNightSystem
    : public afterhours::System<afterhours::tags::All<EntityType::Door>> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, float) override {
        if (!CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                 STORE_BUILDING.bounds)) {
            return;
        }
        entity.addComponentIfMissing<IsSolid>();
    }
};

struct OnNightStartedSystem : public afterhours::System<RespondsToDayNight> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_night_started();
    }
};

struct ReleaseMopBuddyAtStartOfDaySystem
    : public afterhours::System<IsItem,
                                afterhours::tags::All<EntityType::MopBuddy>> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, IsItem& isitem, float) override {
        if (!isitem.is_held()) {
            return;
        }

        OptEntity holder = EntityHelper::getEntityForID(isitem.holder());
        if (!holder) {
            return;
        }

        // Update the holder's CanHoldItem to release the mop buddy
        CanHoldItem& canHold = holder->get<CanHoldItem>();
        if (!canHold.is_holding_item()) {
            return;
        }
        if (canHold.item_id() != entity.id) {
            return;
        }
        canHold.update(nullptr, entity_id::INVALID);

        // Force drop the mop buddy
        isitem.set_held_by(EntityType::Unknown, -1);
    }
};

struct DeleteTrashWhenLeavingPlanningSystem
    : public afterhours::System<IsStoreSpawned> {
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

    virtual void for_each_with(Entity& entity, IsStoreSpawned& isss,
                               float) override {
        (void) isss;  // Unused parameter
        // Only delete if it's not being held
        if (entity.has<IsItem>() && !entity.get<IsItem>().is_held()) {
            entity.cleanup = true;
        }
    }
};

struct ResetRegisterQueueWhenLeavingInRoundSystem
    : public afterhours::System<HasWaitingQueue> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, HasWaitingQueue& hwq, float) override {
        hwq.clear();
    }
};

// Struct definition from reset_has_day_night_changed_system.h moved here

// System that resets the needs_to_process_change flag after all transition
// systems have run
struct ResetHasDayNightChanged : public afterhours::System<HasDayNightTimer> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            // Only run when the flag is set (meaning transition systems ran)
            return timer.needs_to_process_change;
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, HasDayNightTimer& timer,
                               float) override {
        // Reset the flag after all transition systems have run
        timer.needs_to_process_change = false;
    }
};

}  // namespace system_manager

void SystemManager::register_day_night_transition_systems() {
    // Day/night transition systems - all check needs_to_process_change in
    // should_run(), reset clears the flag after processing
    {
#if ENABLE_DEV_FLAGS
        systems.register_update_system(
            std::make_unique<system_manager::BypassInitSystem>());
#endif
        // Day start systems
        {
            systems.register_update_system(
                std::make_unique<system_manager::GenerateStoreOptionsSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OpenStoreDoorsSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::
                        DeleteFloatingItemsWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::TellCustomersToLeaveSystem>());

            systems.register_update_system(
                std::make_unique<
                    system_manager::ResetToiletWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::
                        ResetCustomerSpawnerWhenLeavingInRoundSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::UpdateNewMaxCustomersSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OnNightEndedTriggerSystem>());
            systems.register_update_system(
                std::make_unique<system_manager::OnDayStartedTriggerSystem>());
            systems.register_update_system(
                std::make_unique<
                    system_manager::OnRoundFinishedTriggerSystem>());
#if ENABLE_DEV_FLAGS
            systems.register_update_system(
                std::make_unique<system_manager::BypassRoundTrackerSystem>());
#endif
        }
        systems.register_update_system(
            std::make_unique<system_manager::CleanUpOldStoreOptionsSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::OnDayEndedSystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::ResetRegisterQueueWhenLeavingInRoundSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::CloseBuildingsWhenNightSystem>());
        systems.register_update_system(
            std::make_unique<system_manager::OnNightStartedSystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::ReleaseMopBuddyAtStartOfDaySystem>());
        systems.register_update_system(
            std::make_unique<
                system_manager::DeleteTrashWhenLeavingPlanningSystem>());
    }
    // This one needs to run after the transition systems to clear the flag
    systems.register_update_system(
        std::make_unique<system_manager::ResetHasDayNightChanged>());
}
