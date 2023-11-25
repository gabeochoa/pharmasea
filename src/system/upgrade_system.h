
#pragma once

#include "../components/has_timer.h"
#include "../components/is_round_settings_manager.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {
namespace upgrade {

inline void start_of_day(Entity& entity, float dt) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;

    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    // Remove all
    irsm.daily_upgrades.clear();

    for (const auto& name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(name);
        irsm.daily_upgrades[name] = UpgradeInstance(upgrade);
    }
}

inline void in_round_update(Entity& entity, float dt) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;

    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    //  TODO when you ffwd it skips some of the hours
    //  should we instead run X times at least for acitvities?
    HasTimer& hasTimer = entity.get<HasTimer>();
    int hour = 100 - static_cast<int>(hasTimer.pct() * 100.f);

    if (hour <= irsm.ran_for_hour) return;
    irsm.ran_for_hour = hour;

    log_info("current hour is {}", hour);

    // unapply all previous upgrades
    for (auto& name : irsm.applied_upgrades) {
        irsm.unapply_upgrade_by_name(name);
    }

    for (auto& kv : irsm.daily_upgrades) {
        UpgradeInstance& instance = kv.second;

        // These never apply
        if (instance.parent_copy.duration == 0) {
            continue;
        }

        // TODO still not handling expiration
        if (instance.parent_copy.duration > 0) {
            // log_info("daily and not expired");
            irsm.apply_upgrade_by_name(kv.first);
            continue;
        }

        if (instance.parent_copy.active_hours.test(hour)) {
            // log_info("its time!");
            irsm.apply_upgrade_by_name(kv.first);
            continue;
        }

        // These always apply
        if (instance.parent_copy.duration == -1 &&
            instance.parent_copy.active_hours.all()) {
            // log_info("daily");
            irsm.apply_upgrade_by_name(kv.first);
            continue;
        }

        // log_warn("daily upgrade but didnt hit one of the matching cases {}",
        // kv.first);
    }

    for (ConfigKey& key : irsm.activities) {
        switch (key) {
            case ConfigKey::CustomerSpawn: {
                auto spawner = EntityQuery()
                                   .whereType(EntityType::CustomerSpawner)
                                   .gen_first();
                if (!spawner) {
                    log_warn("Could not find customer spawner?");
                    continue;
                }
                auto& new_ent = EntityHelper::createEntity();
                make_customer(
                    new_ent,
                    SpawnInfo{.location = spawner->get<Transform>().as2(),
                              .is_first_this_round = false},
                    true);
            } break;
            case ConfigKey::Test:
            case ConfigKey::RoundLength:
            case ConfigKey::MaxNumOrders:
            case ConfigKey::PatienceMultiplier:
            case ConfigKey::CustomerSpawnMultiplier:
            case ConfigKey::NumStoreSpawns:
            case ConfigKey::UnlockedToilet:
            case ConfigKey::PissTimer:
            case ConfigKey::BladderSize:
            case ConfigKey::HasCityMultiplier:
            case ConfigKey::DrinkCostMultiplier:
            case ConfigKey::VomitFreqMultiplier:
            case ConfigKey::VomitAmountMultiplier:
            case ConfigKey::DayCount:
            case ConfigKey::Entity:
            case ConfigKey::Drink:
                break;
        }
    }
    irsm.activities.clear();
}

inline void end_of_day(Entity& entity, float dt) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    // for all temp reduce duration and unapply any that are 0
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();
    irsm.ran_for_hour = -1;

    /*
    for (auto& temp_upgrade_kv : irsm.temp_upgrades_applied) {
        auto& temp_upgrade = temp_upgrade_kv.second.parent_copy;
        if (temp_upgrade.duration < 0) continue;

        temp_upgrade.duration--;
        if (temp_upgrade.duration == 0) {
            irsm.unapply_upgrade_by_name(temp_upgrade.name);
            // TODO remove from the map...
            temp_upgrade.duration = -2;
        }
    }
    */
}

}  // namespace upgrade
}  // namespace system_manager
