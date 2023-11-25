
#pragma once

#include "../components/has_timer.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_upgrade_spawned.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {
namespace upgrade {

inline void start_of_day(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;

    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    // Remove all
    irsm.daily_upgrades.clear();

    for (const auto& name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(name);
        UpgradeInstance instance(upgrade);

        if (instance.parent_copy.applied_at_beginning_of_day()) {
            log_info("apply daily upgrade {}", name);
            irsm.apply_upgrade_by_name(instance.parent_copy.name);
            instance.applied_daily = true;
        }

        irsm.daily_upgrades[name] = instance;
    }
}

inline void in_round_update(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;

    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    //  TODO when you ffwd in debug mode it skips some of the hours
    //  should we instead run X times at least for acitvities?
    HasTimer& hasTimer = entity.get<HasTimer>();
    int hour = 100 - static_cast<int>(hasTimer.pct() * 100.f);

    if (hour <= irsm.ran_for_hour) return;
    irsm.ran_for_hour = hour;

    log_info("current hour is {}", hour);

    // Unapply
    for (const auto& kv : irsm.daily_upgrades) {
        const UpgradeInstance& instance = kv.second;

        // applied beginning of day
        if (instance.applied_daily) continue;

        // unapply anything from last hour
        if (contains(irsm.applied_upgrades, kv.first)) {
            irsm.unapply_upgrade_by_name(kv.first);
        }
    }

    // Apply
    for (const auto& kv : irsm.daily_upgrades) {
        const UpgradeInstance& instance = kv.second;

        // applied beginning of day already
        if (instance.applied_daily) continue;

        // These no longer apply
        if (instance.parent_copy.duration == 0) continue;

        // Doesnt apply to this hour
        if (!instance.parent_copy.active_hours.test(hour)) continue;

        irsm.apply_upgrade_by_name(kv.first);
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

inline void end_of_day(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    // for all temp reduce duration and unapply any that are 0
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();
    irsm.ran_for_hour = -1;

    for (auto& kv : irsm.daily_upgrades) {
        UpgradeInstance& instance = kv.second;
        instance.applied_daily = false;
        // TODO delete any tagged items
        if (instance.parent_copy.duration == 0) continue;
        if (instance.parent_copy.duration == -1) continue;
        instance.parent_copy.duration--;
    }

    for (Entity& ent :
         EntityQuery().whereHasComponent<IsUpgradeSpawned>().gen()) {
        ent.cleanup = true;

        // Also cleanup the item its holding if it has one
        if (ent.is_missing<CanHoldItem>()) continue;
        CanHoldItem& chi = ent.get<CanHoldItem>();
        if (!chi.is_holding_item()) continue;
        if (!chi.item()) continue;
        chi.item()->cleanup = true;
    }
}

}  // namespace upgrade
}  // namespace system_manager
