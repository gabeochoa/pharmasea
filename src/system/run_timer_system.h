#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_bank.h"
#include "../components/is_progression_manager.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "progression.h"
#include "system_manager.h"

namespace system_manager {

struct RunTimerSystem : public afterhours::System<HasDayNightTimer> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, HasDayNightTimer& ht,
                               float dt) override {
        ht.pass_time(dt);
        if (!ht.is_round_over()) return;

        if (ht.is_daytime()) {
            ht.start_night();

            // TODO we assume that this entity is the same one which should be
            // the case if so then we should be using the System filtering to
            // just grab that for now lets just leave this and we can later
            // bring it in
            if (entity.is_missing<IsBank>())
                log_warn("system_manager::run_timer missing IsBank");

            if (ht.days_until() <= 0) {
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

            return;
        }

        // TODO we assume that this entity is the same one which should be the
        // case if so then we should be using the System filtering to just grab
        // that for now lets just leave this and we can later bring it in
        if (entity.is_missing<IsProgressionManager>())
            log_warn("system_manager::run_timer missing IsProgressionManager");
        progression::collect_progression_options(entity, dt);

        // TODO theoretically we shouldnt start until after you choose upgrades
        // but we are gonna change how this works later anyway i think
        ht.start_day();
    }
};

}  // namespace system_manager
