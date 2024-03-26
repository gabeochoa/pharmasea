
#include "upgrades.h"

#include "ingredient.h"
#include "settings.h"

std::shared_ptr<UpgradeImpl> make_upgrade(UpgradeClass uc) {
    UpgradeImpl* ptr = nullptr;

    switch (uc) {
        case UpgradeClass::LongerDay:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Longer Day",
                .icon_name = "longer_day",
                .flavor_text = "Find an extra couple hours in the day.",
                .description = "(Makes the day twice as long)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        {
                            float val =
                                config.get<float>(ConfigKey::RoundLength);
                            config.set<float>(ConfigKey::RoundLength, val * 2);
                        }
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    // TODO store somewhere that we are unlocking this
                    return true;
                }};
            break;
        case UpgradeClass::UnlockToilet:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Gotta go",
                .icon_name = "gotta_go",
                .flavor_text = "Drinking has its consequences.",
                .description =
                    "(Customers will order twice, but will need to go to "
                    "the "
                    "bathroom)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        {
                            int val = config.get<int>(ConfigKey::MaxNumOrders);
                            config.set<int>(ConfigKey::MaxNumOrders, val * 2);
                        }
                        config.set<bool>(ConfigKey::UnlockedToilet, true);

                        // TODO ipm unlocked entity types?
                        // TODO add supoprt for OnUpgrade (see entity_type.h)
                        config.store_to_spawn.push_back(EntityType::Toilet);
                        config.forever_required.push_back(EntityType::Toilet);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.get<bool>(ConfigKey::UnlockedToilet) == false;
                }};
            break;
        case UpgradeClass::BigBladders:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Big Bladders",
                .icon_name = "big_bladders",
                .flavor_text = "I've been training..",
                .description =
                    "(Customers will go too the bathroom less often but for "
                    "longer)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        {
                            int val = config.get<int>(ConfigKey::PissTimer);
                            config.set<int>(ConfigKey::PissTimer, val * 2);
                        }
                        {
                            int val = config.get<int>(ConfigKey::BladderSize);
                            config.set<int>(ConfigKey::BladderSize, val * 2);
                        }
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.get<bool>(ConfigKey::UnlockedToilet) == true;
                }};
            break;
        case UpgradeClass::BigCity:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Big City",
                .icon_name = "big_city",
                .flavor_text = "I'm walking 'ere.",
                .description = "(Customers pay double but have less patience)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        {
                            float val = config.get<float>(
                                ConfigKey::PatienceMultiplier);
                            config.set<float>(ConfigKey::PatienceMultiplier,
                                              val * 0.5f);
                        }
                        {
                            float val = config.get<float>(
                                ConfigKey::DrinkCostMultiplier);
                            config.set<float>(ConfigKey::DrinkCostMultiplier,
                                              val * 2.0f);
                        }
                        config.set<bool>(ConfigKey::HasCityMultiplier, true);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.get<bool>(ConfigKey::HasCityMultiplier) ==
                           false;
                }};
            break;
        case UpgradeClass::SmallTown:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "SmallTown",
                .icon_name = "small_town",
                .flavor_text = "This town ain't big enough for the two of us.",
                .description = "(Customers pay half but have double patience)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        {
                            float val = config.get<float>(
                                ConfigKey::PatienceMultiplier);
                            config.set<float>(ConfigKey::PatienceMultiplier,
                                              val * 2.0f);
                        }
                        {
                            float val = config.get<float>(
                                ConfigKey::DrinkCostMultiplier);
                            config.set<float>(ConfigKey::DrinkCostMultiplier,
                                              val * 0.5f);
                        }
                        config.set<bool>(ConfigKey::HasCityMultiplier, true);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.get<bool>(ConfigKey::HasCityMultiplier) ==
                           false;
                }};
            break;
        case UpgradeClass::Champagne:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Champagne",
                .icon_name = "upgrade_champagne",
                .flavor_text = "Someone get the bandaids.",
                .description =
                    "(Unlocks Champagne which needs to be saber'd before "
                    "pouring)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        ipm.unlock_drink(champagne);

                        config.store_to_spawn.push_back(
                            EntityType::ChampagneHolder);
                        config.forever_required.push_back(
                            EntityType::ChampagneHolder);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager& ipm) -> bool {
                    return !ipm.is_drink_unlocked(champagne);
                }};
            break;
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }

    return std::shared_ptr<UpgradeImpl>(ptr);
}
