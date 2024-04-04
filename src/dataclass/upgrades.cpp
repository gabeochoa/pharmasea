
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
                .name = text_lookup("Longer Day"),
                .icon_name = "longer_day",
                .flavor_text =
                    text_lookup("Find an extra couple hours in the day."),
                .description = text_lookup("(Makes the day twice as long)"),
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
                .name = text_lookup("Gotta go"),
                .icon_name = "gotta_go",
                .flavor_text = text_lookup("Drinking has its consequences."),
                .description = text_lookup(
                    "(Customers will order twice, but will need to go to "
                    "the "
                    "bathroom)"),
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
                .name = text_lookup("Big Bladders"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("I've been training.."),
                .description = text_lookup(
                    "(Customers will go too the bathroom less often but for "
                    "longer)"),
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
                .name = text_lookup("Speakeasy"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup(
                    "first you gotta get a fish, then bring it to the..."),
                .description = text_lookup(
                    "(Half the customers, but they will pay 1% more for "
                    "every extra tile they have to visit before finding the "
                    "register)"),
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
                .name = text_lookup("Main Street"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("location, location, location."),
                .description =
                    text_lookup("(Twice as many Customers will visit)"),
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
                .name = text_lookup("Big City"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("I'm walking 'ere."),
                .description = text_lookup(
                    "(Customers pay double but have less patience)"),
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
                .name = text_lookup("SmallTown"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup(
                    "This town ain't big enough for the two of us."),
                .description = text_lookup(
                    "(Customers pay half but have double patience)"),
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
                .name = text_lookup("Champagne"),
                .icon_name = "upgrade_champagne",
                .flavor_text = text_lookup("Someone get the bandaids."),
                .description = text_lookup(
                    "(Unlocks Champagne which needs to be saber'd before "
                    "pouring)"),
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
                .name = text_lookup("Happy Hour"),
                .icon_name = "happy_hour",
                .flavor_text = text_lookup("all the buzz at half the price"),
                .description = text_lookup(
                    "(more people at the beginning of the day but cheaper "
                    "drinks)"),
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
                .name = text_lookup("Pitcher"),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("its actually a carafe."),
                .description = text_lookup(
                    "(Unlocks a pitcher which can store up to 10 of the same "
                    "drink)"),
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
            // TODO add a way for the other people to be spawned and not order
            // but take up space in the line?
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("me and the boys"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text =
                    text_lookup("crackin open a cold one or two... or ten ."),
                .description = text_lookup(
                    "(Customers will now order a pitcher of 10 beers)"),
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
        case UpgradeClass::Mocktails:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("Mocktails"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("does this taste weak to you?"),
                .description = text_lookup(
                    "(You can forget one alcohol in a recipe but customers "
                    "will order more to make up for it)"),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        // TODO do they only order more when you mess up?
                        // TODO do they not vomit if you forget alcohol?

                        // People order more because they want the same buzz
                        config.permanently_modify<int>(
                            ConfigKey::MaxNumOrders, Operation::Multiplier, 2);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
        case UpgradeClass::HeavyHanded:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("HeavyHanded"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text =
                    text_lookup("Oops, hopefully you have a ride home"),
                // TODO make it so that you have to add one more alcohol
                // to get the effect?
                .description = text_lookup("(less profit & more vomit, but "
                                           "customers will order more)"),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<int>(
                            ConfigKey::MaxNumOrders, Operation::Multiplier, 2);

                        config.permanently_modify<float>(
                            ConfigKey::VomitFreqMultiplier,
                            Operation::Multiplier, 2);

                        config.permanently_modify<float>(
                            ConfigKey::VomitAmountMultiplier,
                            Operation::Multiplier, 2);

                        config.permanently_modify<float>(
                            ConfigKey::DrinkCostMultiplier,
                            Operation::Multiplier, 0.75);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
        case UpgradeClass::PottyProtocol:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("Potty Protocol"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("aim for the bowl"),
                .description = text_lookup(
                    "(before vomiting customers will try to find an empty "
                    "toilet)"),
                .onUnlock = [](ConfigData&, IsProgressionManager&) {},
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return bitset_utils::test(config.unlocked_upgrades,
                                              UpgradeClass::UnlockToilet);
                }};
            break;
        case UpgradeClass::SippyCups:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("Sippy Cups"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("savor the flavor"),
                .description = text_lookup(
                    "(customers will take twice as long to drink and order "
                    "less)"),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::MaxDrinkTime, Operation::Multiplier,
                            2.f);
                        // TODO consider order of applying permanent upgrades
                        // for keys that have minimums since the default orders
                        // is 1, we need to require the current value is more
                        // than that otherwise we might see different values
                        // based on order unlocked
                        config.permanently_modify<int>(ConfigKey::MaxNumOrders,
                                                       Operation::Divide, 2);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // See TODO above
                    return config.get<int>(ConfigKey::MaxNumOrders) >= 2;
                }};
            break;
        case UpgradeClass::DownTheHatch:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = text_lookup("Down The Hatch"),
                // TODO
                .icon_name = "upgrade_default",
                .flavor_text = text_lookup("chug chug chug chug!"),
                // TODO do they also order more?
                .description = text_lookup("(customers will drink faster)"),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        config.permanently_modify<float>(
                            ConfigKey::MaxDrinkTime, Operation::Multiplier,
                            0.5f);
                        config.permanently_modify<int>(
                            ConfigKey::MaxNumOrders, Operation::Multiplier, 2);
                    },
                .meetsPrereqs = [](const ConfigData&,
                                   const IsProgressionManager&) -> bool {
                    return true;
                }};
            break;
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }

    return std::shared_ptr<UpgradeImpl>(ptr);
}
