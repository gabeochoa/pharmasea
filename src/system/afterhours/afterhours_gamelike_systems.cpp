#include "../../ah.h"
#include "../../components/can_hold_item.h"
#include "../../components/can_pathfind.h"
#include "../../components/collects_customer_feedback.h"
#include "../../components/has_day_night_timer.h"
#include "../../components/is_bank.h"
#include "../../components/is_item_container.h"
#include "../../components/is_pnumatic_pipe.h"
#include "../../components/is_progression_manager.h"
#include "../../components/is_round_settings_manager.h"
#include "../../engine/log.h"
#include "../../engine/runtime_globals.h"
#include "../../engine/statemanager.h"
#include "../../entity_helper.h"
#include "../../entity_query.h"
#include "../../entity_type.h"
#include "../../globals.h"
#include "../../vec_util.h"
#include "afterhours_systems.h"
#include "../ai/ai_transition_systems.h"
#include "../ai/process_ai_system.h"
#include "../helpers/progression.h"
#include "../core/sophie.h"
#include "../core/system_manager.h"

namespace system_manager {

// System for processing containers that should backfill items during gamelike
// updates
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

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, IsItemContainer& iic,
                               CanHoldItem& canHold, float dt) override {
        (void) iic;      // Unused parameter
        (void) canHold;  // Unused parameter
        process_is_container_and_should_backfill_item(entity, dt);
    }
};

struct RunTimerSystem : public afterhours::System<HasDayNightTimer> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void execute_close_bar(Entity& entity, HasDayNightTimer& ht) {
        ht.close_bar();
        if (entity.is_missing<IsBank>())
            log_error("system_manager::run_timer missing IsBank");
        // Upgrading this log to an error because it should never happen
        // and if it never happens we can add the system filtering

        // If we have more days until rent is due, we can skip the rent payment
        if (ht.days_until() > 0) {
            return;
        }

        IsBank& isbank = entity.get<IsBank>();
        if (isbank.balance() < ht.rent_due()) {
            log_error("you ran out of money, sorry");
        }

        isbank.withdraw(ht.rent_due());
        ht.reset_rent_days();
        // TODO update rent due amount
        ht.update_amount_due(ht.rent_due() + 25);

        // TODO add a way to pay ahead of time ?? design
    }

    void execute_open_bar(Entity& entity, HasDayNightTimer& ht, float dt) {
        if (entity.is_missing<IsProgressionManager>())
            log_error("system_manager::run_timer missing IsProgressionManager");
        progression::collect_progression_options(entity, dt);
        // TODO theoretically we shouldnt start until after you choose upgrades
        // but we are gonna change how this works later anyway i think
        ht.open_bar();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               float dt) override {
        ht.pass_time(dt);
        if (!ht.is_round_over()) return;
        ht.is_bar_open() ? execute_close_bar(entity, ht)
                         : execute_open_bar(entity, ht, dt);
    }
};

struct ProcessPnumaticPipePairingSystem
    : public afterhours::System<IsPnumaticPipe> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp,
                               float) override {
        if (ipp.has_pair()) return;

        OptEntity other_pipe = EntityQuery()  //
                                   .whereNotMarkedForCleanup()
                                   .whereNotID(entity.id)
                                   .whereHasComponent<IsPnumaticPipe>()
                                   .whereLambda([](const Entity& pipe) {
                                       const IsPnumaticPipe& otherpp =
                                           pipe.get<IsPnumaticPipe>();
                                       // Find only the ones that dont have a
                                       // pair
                                       return !otherpp.has_pair();
                                   })
                                   .gen_first();

        if (other_pipe.has_value()) {
            IsPnumaticPipe& otherpp = other_pipe->get<IsPnumaticPipe>();
            otherpp.paired.set_id(entity.id);
            ipp.paired.set_id(other_pipe->id);
        }

        // still dont have a pair, we probably just have an odd number
        if (!ipp.has_pair()) return;
    }
};

struct PassTimeForTransactionAnimationSystem
    : public afterhours::System<IsBank> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity&, IsBank& bank, float dt) override {
        std::vector<IsBank::Transaction>& transactions =
            bank.get_transactions();

        // Remove any old ones
        remove_all_matching<IsBank::Transaction>(
            transactions, [](const IsBank::Transaction& transaction) {
                return transaction.remainingTime <= 0.f ||
                       transaction.amount == 0;
            });

        if (transactions.empty()) return;

        IsBank::Transaction& transaction = bank.get_next_transaction();
        transaction.remainingTime -= dt;
    }
};

namespace sophie {
// Forward declarations for sophie namespace functions
void customers_in_store(Entity& entity);
void holding_stolen_item(Entity& entity);
void garbage_in_store(Entity& entity);
void bar_not_clean(Entity& entity);
void overlapping_furniture(Entity& entity);
void forgot_item_in_spawn_area(Entity& entity);
void deleting_item_needed_for_recipe(Entity& entity);
void lightweight_map_validation(Entity& entity);
}  // namespace sophie

struct EndOfRoundCompletionValidationSystem
    : public afterhours::System<HasDayNightTimer, CollectsCustomerFeedback,
                                afterhours::tags::All<EntityType::Sophie>> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               CollectsCustomerFeedback& feedback,
                               float dt) override {
        const auto debug_mode_on = globals::debug_ui_enabled();

        // TODO i dont like that this is copy paste from layers/round_end
        if (SystemManager::get().is_bar_closed() &&
            ht.get_current_length() > 0 && !debug_mode_on)
            return;

        if (!feedback.waiting_time_pass(dt)) {
            return;
        }

        // doing it this way so that if we wanna make them return bool itll be
        // easy
        typedef std::function<void(Entity&)> WaitingFn;

        std::vector<WaitingFn> fns{
            sophie::customers_in_store,   //
            sophie::holding_stolen_item,  //
            sophie::garbage_in_store,     //
            // sophie::player_holding_furniture,         //
            sophie::bar_not_clean,                    //
            sophie::overlapping_furniture,            //
            sophie::forgot_item_in_spawn_area,        //
            sophie::deleting_item_needed_for_recipe,  //
            sophie::lightweight_map_validation,
        };

        for (const auto& fn : fns) {
            fn(entity);
        }
    }
};

}  // namespace system_manager

void SystemManager::register_gamelike_systems() {
    systems.register_update_system(
        std::make_unique<system_manager::RunTimerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessPnumaticPipePairingSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldBackfillItemSystem>());
    systems.register_update_system(
        std::make_unique<
            system_manager::PassTimeForTransactionAnimationSystem>());

    // AI systems: setup -> state processing -> commit
    system_manager::register_ai_transition_systems(systems);
    system_manager::register_ai_systems(systems);
    system_manager::register_ai_transition_commit_systems(systems);

    systems.register_update_system(
        std::make_unique<
            system_manager::EndOfRoundCompletionValidationSystem>());

    register_day_night_transition_systems();
}