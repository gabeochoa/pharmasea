
#include "upgrades.h"

#include "../engine/bitset_utils.h"
#include "ingredient.h"
#include "settings.h"

[[nodiscard]] bool has_city_upgrade(const ConfigData& config) {
    return bitset_utils::test(config.unlocked_upgrades,
                              UpgradeClass::SmallTown) ||
           bitset_utils::test(config.unlocked_upgrades, UpgradeClass::BigCity);
}

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
                        config.permanently_modify<float>(
                            ConfigKey::RoundLength, Operation::Multiplier, 2.f);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
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
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        config.permanently_modify<int>(ConfigKey::MaxNumOrders,
                                                       Operation::Multiplier,
                                                       2.f);

                        {
                            ipm.unlock_entity(EntityType::Toilet);
                            config.store_to_spawn.push_back(EntityType::Toilet);
                            config.forever_required.push_back(
                                EntityType::Toilet);
                        }
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
        case UpgradeClass::BigBladders:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Big Bladders",
                // TODO icon
                .icon_name = "upgrade_default",
                .flavor_text = "I've been training..",
                .description =
                    "(Customers will go too the bathroom less often but for "
                    "longer)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<int>(
                            ConfigKey::PissTimer, Operation::Multiplier, 2);
                        config.permanently_modify<int>(
                            ConfigKey::BladderSize, Operation::Multiplier, 2);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return bitset_utils::test(config.unlocked_upgrades,
                                              UpgradeClass::UnlockToilet);
                }};
            break;
        case UpgradeClass::Speakeasy:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Speakeasy",
                // TODO icon
                .icon_name = "upgrade_default",
                .flavor_text =
                    "first you gotta get a fish, then bring it to the...",
                .description =
                    "(Half the customers, but they will pay 1% more for "
                    "every extra tile they have to visit before finding the "
                    "register)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::CustomerSpawnMultiplier,
                            Operation::Multiplier, 0.5f);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
        case UpgradeClass::MainStreet:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Main Street",
                // TODO icon
                .icon_name = "upgrade_default",
                .flavor_text = "location, location, location.",
                .description = "(Twice as many Customers will visit)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::CustomerSpawnMultiplier,
                            Operation::Multiplier, 2.0f);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
        case UpgradeClass::BigCity:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Big City",
                // TODO icon
                .icon_name = "upgrade_default",
                .flavor_text = "I'm walking 'ere.",
                .description = "(Customers pay double but have less patience)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::PatienceMultiplier,
                            Operation::Multiplier, 0.5f);
                        config.permanently_modify<float>(
                            ConfigKey::DrinkCostMultiplier,
                            Operation::Multiplier, 2.0f);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return !has_city_upgrade(config);
                }};
            break;
        case UpgradeClass::SmallTown:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "SmallTown",
                // TODO icon
                .icon_name = "upgrade_default",
                .flavor_text = "This town ain't big enough for the two of us.",
                .description = "(Customers pay half but have double patience)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::PatienceMultiplier,
                            Operation::Multiplier, 2.0f);
                        config.permanently_modify<float>(
                            ConfigKey::DrinkCostMultiplier,
                            Operation::Multiplier, 0.5f);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return !has_city_upgrade(config);
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

                        // TODO should these be one function call somehow?
                        {
                            ipm.unlock_entity(EntityType::ChampagneHolder);
                            config.store_to_spawn.push_back(
                                EntityType::ChampagneHolder);
                            config.forever_required.push_back(
                                EntityType::ChampagneHolder);
                        }
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager& ipm) -> bool {
                    return !ipm.is_drink_unlocked(champagne);
                }};
            break;

        case UpgradeClass::HappyHour:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Happy Hour",
                .icon_name = "happy_hour",
                .flavor_text = "all the buzz at half the price",
                .description =
                    "(more people at the beginning of the day but cheaper "
                    "drinks)",
                .onUnlock = [](ConfigData&, IsProgressionManager&) {},
                .onHourMods = [](const ConfigData&, const IsProgressionManager&,
                                 int hour) -> Mods {
                    Mods mods;
                    // no modification during those hours
                    if (hour > 15 || hour < 10) return mods;

                    mods.push_back(UpgradeModification{
                        .name = ConfigKey::DrinkCostMultiplier,
                        .operation = Operation::Multiplier,
                        .value = 0.75f,
                    });
                    return mods;
                },
                .onHourActions = [](const ConfigData&,
                                    const IsProgressionManager&,
                                    int hour) -> Actions {
                    Actions actions;
                    if (hour > 15 || hour < 10) return actions;
                    actions.push_back(UpgradeAction::SpawnCustomer);
                    return actions;
                },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;

        case UpgradeClass::Pitcher:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Pitcher",
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = "its actually a carafe.",
                .description =
                    "(Unlocks a pitcher which can store up to 10 of the same "
                    "drink)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        ipm.unlock_entity(EntityType::PitcherCupboard);
                        config.store_to_spawn.push_back(
                            EntityType::PitcherCupboard);
                        // We explicitly dont mark it required, until
                        // the next upgrade "me and the boys"
                        // where it actually is required
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager& ipm) -> bool {
                    return ipm.is_drink_unlocked(beer);
                }};
            break;
        case UpgradeClass::MeAndTheBoys:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "me and the boys",
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = "crackin open a cold one or two... or ten .",
                .description =
                    "(Customers will now order a pitcher of 10 beers)",
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        ipm.unlock_drink(beer_pitcher);

                        config.forever_required.push_back(
                            EntityType::PitcherCupboard);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // Require that you have normal pitcher unlocked first
                    return bitset_utils::test(config.unlocked_upgrades,
                                              UpgradeClass::Pitcher);
                }};
            break;
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }

    return std::shared_ptr<UpgradeImpl>(ptr);
}
