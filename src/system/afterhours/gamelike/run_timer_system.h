#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_bank.h"
#include "../../../components/is_progression_manager.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../../core/system_manager.h"
#include "../../helpers/progression.h"

namespace system_manager {

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

}  // namespace system_manager