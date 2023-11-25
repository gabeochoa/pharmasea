
#pragma once

#include "../components/has_timer.h"
#include "../components/is_free_in_store.h"
#include "../components/is_round_settings_manager.h"
#include "../entity_helper.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {
namespace upgrade {
enum UpgradeTimeOfDay {
    Unlock,
    Daily,
    Hour,
};

inline void execute_activites(IsRoundSettingsManager& irsm,
                              UpgradeTimeOfDay timeofday,
                              std::vector<ActivityOutcome> activities) {
    for (auto& activity : activities) {
        switch (activity.name) {
            case ConfigKey::Entity: {
                EntityType et = std::get<EntityType>(activity.value);
                switch (timeofday) {
                    case Unlock: {
                        bitset_utils::set(irsm.on_unlock_entities, et);

                        OptEntity spawn_area =
                            EntityHelper::getMatchingFloorMarker(
                                // Note we spawn free items in the purchase area
                                // so its more obvious that they are free
                                IsFloorMarker::Type::Store_PurchaseArea);
                        if (!spawn_area) {
                            // TODO need to guarantee this exists long before we
                            // get here
                            log_error("Could not find spawn area entity");
                        }

                        auto& entity = EntityHelper::createEntity();
                        entity.addComponent<IsFreeInStore>();
                        convert_to_type(et, entity,
                                        spawn_area->get<Transform>().as2());
                    } break;
                    case Daily:
                        log_warn(
                            "What does it mean to unlock an entity for a day?");
                        bitset_utils::set(irsm.on_daily_entities, et);
                        break;
                    case Hour:
                        log_warn(
                            "What does it mean to unlock an entity for an "
                            "hour?");
                        bitset_utils::set(irsm.on_hour_entities, et);
                        break;
                }
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
            case ConfigKey::Drink:
                log_warn(
                    "you have an activity with key {}, not sure how to handle "
                    "that",
                    magic_enum::enum_name<ConfigKey>(activity.name));
                break;
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
        }
    }
}

inline void on_round_started(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    log_info("upgrade on round started ");

    for (const auto& upgrade_name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(upgrade_name);
        for (auto& effect : upgrade.on_start_of_day) {
            irsm.apply_effect(effect);
        }
    }
}

inline void on_round_finished(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    log_info("upgrade on round finished");

    irsm.ran_for_hour = -1;

    for (const auto& upgrade_name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(upgrade_name);
        for (auto& effect : upgrade.on_start_of_day) {
            irsm.unapply_effect(effect);
        }
    }

    // Turn off any upgrades that were active at hour 100
    for (const auto& upgrade_name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(upgrade_name);
        for (auto& hourly_effect_pair : upgrade.hourly_effects) {
            bool active_last_hour = hourly_effect_pair.first.test(100);
            if (!active_last_hour) continue;

            for (auto& effect : hourly_effect_pair.second) {
                irsm.unapply_effect(effect);
            }
        }
    }
}

inline void in_round_update(Entity& entity, float dt) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    //  TODO when you ffwd in debug mode it skips some of the hours
    //  should we instead run X times at least for acitvities?
    HasTimer& hasTimer = entity.get<HasTimer>();
    int hour = 100 - static_cast<int>(hasTimer.pct() * 100.f);

    // Make sure we only run this once an hour
    if (hour <= irsm.ran_for_hour) return;
    irsm.ran_for_hour = hour;

    log_info("upgrade on round uprade hour {} ", hour);

    for (const auto& upgrade_name : irsm.unlocked_upgrades) {
        Upgrade upgrade = UpgradeLibrary::get().get(upgrade_name);
        for (auto& hourly_effect_pair : upgrade.hourly_effects) {
            // Unapply everything from last hour
            bool active_last_hour =
                (hour != 0) && hourly_effect_pair.first.test(hour - 1);
            if (active_last_hour) {
                for (auto& effect : hourly_effect_pair.second) {
                    irsm.unapply_effect(effect);
                }
            }

            // If this effect is not active during this hour, skip
            bool active_this_hour = hourly_effect_pair.first.test(hour);
            if (!active_this_hour) {
                continue;
            }

            // Apply the effects for this hour
            for (auto& effect : hourly_effect_pair.second) {
                irsm.apply_effect(effect);
            }

            // Apply activity outcomes,
            execute_activites(irsm, UpgradeTimeOfDay::Hour, irsm.activities);
        }
    }
}

}  // namespace upgrade
}  // namespace system_manager
