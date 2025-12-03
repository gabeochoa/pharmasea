#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_type.h"

namespace system_manager {
struct UpgradeInRoundUpdateSystem
    : public afterhours::System<IsRoundSettingsManager, IsProgressionManager,
                                HasDayNightTimer> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_nighttime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, IsRoundSettingsManager& irsm,
                               IsProgressionManager& ipm,
                               HasDayNightTimer& hasTimer, float) override {
        int hour = 100 - static_cast<int>(hasTimer.pct() * 100.f);

        // Make sure we only run this once an hour
        if (hour <= irsm.ran_for_hour) return;

        int hours_missed = (hour - irsm.ran_for_hour);
        if (hours_missed > 1) {
            // 1 means normal, so >1 means we actually missed one
            // this currently only happens in debug mode so lets just log it
            log_warn("missed {} hours", hours_missed);

            //  TODO when you ffwd in debug mode it skips some of the hours
            //  should we instead run X times at least for acitvities?
        }

        const auto& collect_mods = [&]() {
            Mods mods;
            for (const auto& upgrade : irsm.selected_upgrades) {
                if (!upgrade->onHourMods) continue;
                auto new_mods = upgrade->onHourMods(irsm.config, ipm, hour);
                mods.insert(mods.end(), new_mods.begin(), new_mods.end());
            }
            irsm.config.this_hours_mods = mods;
        };
        collect_mods();

        // Run actions...
        // we need to run for every hour we missed

        const auto spawn_customer_action = []() {
            auto spawner =
                EQ().whereType(EntityType::CustomerSpawner).gen_first();
            if (!spawner) {
                log_warn("Could not find customer spawner?");
                return;
            }
            auto& new_ent = EntityHelper::createEntity();
            make_customer(new_ent,
                          SpawnInfo{.location = spawner->get<Transform>().as2(),
                                    .is_first_this_round = false},
                          true);
            return;
        };

        for (const auto& upgrade : irsm.selected_upgrades) {
            if (!upgrade->onHourActions) continue;

            // We start at 1 since its normal to have 1 hour missed ^^ see
            // above
            int i = 1;
            while (i < hours_missed) {
                log_info("running actions for {} for hour {} (currently {})",
                         upgrade->name.debug(), irsm.ran_for_hour + i, hour);
                i++;

                auto actions = upgrade->onHourActions(irsm.config, ipm,
                                                      irsm.ran_for_hour + i);
                for (auto action : actions) {
                    switch (action) {
                        case SpawnCustomer:
                            spawn_customer_action();
                            break;
                    }
                }
            }
        }

        irsm.ran_for_hour = hour;
    }
};
}  // namespace system_manager