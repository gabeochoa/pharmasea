
#include "upgrades.h"

#include "../engine/bitset_utils.h"
#include "ingredient.h"
#include "settings.h"
#include "upgrade_class.h"

[[nodiscard]] bool has_city_upgrade(const ConfigData& config) {
    return config.has_upgrade_unlocked(UpgradeClass::SmallTown) ||
           config.has_upgrade_unlocked(UpgradeClass::BigCity);
}

std::shared_ptr<UpgradeImpl> make_upgrade(UpgradeClass uc) {
    UpgradeImpl* ptr = nullptr;

    switch (uc) {
        case UpgradeClass::LongerDay:
            ptr = new UpgradeImpl{
                .type = uc,
                .name =
                    TODO_TRANSLATE("Longer Day", TodoReason::SubjectToChange),
                .icon_name = "longer_day",
                .flavor_text =
                    TODO_TRANSLATE("Find an extra couple hours in the day.",
                                   TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE("(Makes the day twice as long)",
                                              TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("Gotta go", TodoReason::SubjectToChange),
                .icon_name = "gotta_go",
                .flavor_text = TODO_TRANSLATE("Drinking has its consequences.",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Customers will order twice, but will need to go to "
                    "the bathroom)",
                    TodoReason::SubjectToChange),
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
                .name =
                    TODO_TRANSLATE("Big Bladders", TodoReason::SubjectToChange),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text = TODO_TRANSLATE("I've been training..",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Customers will go too the bathroom less often but for "
                    "longer)",
                    TodoReason::SubjectToChange),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        // TODO if you accidentally choose the wrong type
                        // this line will just crash but one once you choose the
                        // upgrade can we catch this at compile time or load
                        // time?
                        // maybe its worth just hardtyping the config keys...
                        config.permanently_modify<float>(
                            ConfigKey::PissTimer, Operation::Multiplier, 2.f);
                        config.permanently_modify<int>(
                            ConfigKey::BladderSize, Operation::Multiplier, 2);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.has_upgrade_unlocked(
                        UpgradeClass::UnlockToilet);
                }};
            break;
        case UpgradeClass::Speakeasy:
            ptr = new UpgradeImpl{
                .type = uc,
                .name =
                    TODO_TRANSLATE("Speakeasy", TodoReason::SubjectToChange),
                .icon_name = "speakeasy",
                .flavor_text = TODO_TRANSLATE(
                    "first you gotta get a fish, then bring it to the...",
                    TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Half the customers, but they will pay 1% more for "
                    "every extra tile they have to visit before finding the "
                    "register)",
                    TodoReason::SubjectToChange),
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
                .name =
                    TODO_TRANSLATE("Main Street", TodoReason::SubjectToChange),
                .icon_name = "main_street",
                .flavor_text = TODO_TRANSLATE("location, location, location.",
                                              TodoReason::SubjectToChange),
                .description =
                    TODO_TRANSLATE("(Twice as many Customers will visit)",
                                   TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("Big City", TodoReason::SubjectToChange),
                .icon_name = "big_city",
                .flavor_text = TODO_TRANSLATE("I'm walking 'ere.",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Customers pay double but have less patience)",
                    TodoReason::SubjectToChange),
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
                .name =
                    TODO_TRANSLATE("SmallTown", TodoReason::SubjectToChange),
                .icon_name = "small_town",
                .flavor_text = TODO_TRANSLATE(
                    "This town ain't big enough for the two of us.",
                    TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Customers pay half but have double patience)",
                    TodoReason::SubjectToChange),
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
                .name =
                    TODO_TRANSLATE("Champagne", TodoReason::SubjectToChange),
                .icon_name = "upgrade_champagne",
                .flavor_text = TODO_TRANSLATE("Someone get the bandaids.",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Unlocks Champagne which needs to be saber'd before "
                    "pouring)",
                    TodoReason::SubjectToChange),
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
                .name =
                    TODO_TRANSLATE("Happy Hour", TodoReason::SubjectToChange),
                .icon_name = "happy_hour",
                .flavor_text = TODO_TRANSLATE("all the buzz at half the price",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(more people at the beginning of the day but cheaper "
                    "drinks)",
                    TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("Pitcher", TodoReason::SubjectToChange),
                .icon_name = "pitcher",
                .flavor_text = TODO_TRANSLATE("its actually a carafe.",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Unlocks a pitcher which can store up to 10 of the same "
                    "drink)",
                    TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("me and the boys",
                                       TodoReason::SubjectToChange),
                .icon_name = "me_and_the_boys",
                .flavor_text =
                    TODO_TRANSLATE("crackin open a cold one or two... or ten .",
                                   TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(Customers will now order a pitcher of 10 beers)",
                    TodoReason::SubjectToChange),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        ipm.unlock_drink(beer_pitcher);

                        config.forever_required.push_back(
                            EntityType::PitcherCupboard);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // Require that you have normal pitcher unlocked first
                    return config.has_upgrade_unlocked(UpgradeClass::Pitcher);
                }};
            break;
        case UpgradeClass::Mocktails:
            ptr = new UpgradeImpl{
                .type = uc,
                .name =
                    TODO_TRANSLATE("Mocktails", TodoReason::SubjectToChange),
                .icon_name = "mocktails",
                .flavor_text = TODO_TRANSLATE("does this taste weak to you?",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(You can forget one alcohol in a recipe but customers "
                    "will order more to make up for it)",
                    TodoReason::SubjectToChange),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager&) {
                        // TODO do they only order more when you mess up?
                        // TODO do they not vomit if you forget alcohol?

                        // People order more because they want the same buzz
                        config.permanently_modify<int>(
                            ConfigKey::MaxNumOrders, Operation::Multiplier, 2);
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // Make sure they dont have the other ingredient one
                    return !config.has_upgrade_unlocked(
                        UpgradeClass::CantEvenTell);
                }};
            break;
        case UpgradeClass::HeavyHanded:
            ptr = new UpgradeImpl{
                .type = uc,
                .name =
                    TODO_TRANSLATE("HeavyHanded", TodoReason::SubjectToChange),
                .icon_name = "heavy_handed",
                .flavor_text =
                    TODO_TRANSLATE("Oops, hopefully you have a ride home",
                                   TodoReason::SubjectToChange),
                // TODO make it so that you have to add one more alcohol
                // to get the effect?
                .description = TODO_TRANSLATE("(less profit & more vomit, but "
                                              "customers will order more)",
                                              TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("Potty Protocol",
                                       TodoReason::SubjectToChange),
                .icon_name = "potty_protocol",
                .flavor_text = TODO_TRANSLATE("aim for the bowl",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(before vomiting customers will try to find an empty "
                    "toilet)",
                    TodoReason::SubjectToChange),
                .onUnlock = [](ConfigData&, IsProgressionManager&) {},
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    return config.has_upgrade_unlocked(
                        UpgradeClass::UnlockToilet);
                }};
            break;
        case UpgradeClass::SippyCups:
            ptr = new UpgradeImpl{
                .type = uc,
                .name =
                    TODO_TRANSLATE("Sippy Cups", TodoReason::SubjectToChange),
                .icon_name = "sippy_cups",
                .flavor_text = TODO_TRANSLATE("savor the flavor",
                                              TodoReason::SubjectToChange),
                .description = TODO_TRANSLATE(
                    "(customers will take twice as long to drink and order "
                    "less)",
                    TodoReason::SubjectToChange),
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
                .name = TODO_TRANSLATE("Down The Hatch",
                                       TodoReason::SubjectToChange),
                .icon_name = "down_the_hatch",
                .flavor_text = TODO_TRANSLATE("chug chug chug chug!",
                                              TodoReason::SubjectToChange),
                // TODO do they also order more?
                .description = TODO_TRANSLATE("(customers will drink faster)",
                                              TodoReason::SubjectToChange),
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
            // TODO what if while the song is playing, that persons patience
            // doesnt drain? or what if playing the music refils their patience
            // (slowly?)
        case UpgradeClass::Jukebox:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = TODO_TRANSLATE("Jukebox", TodoReason::SubjectToChange),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text =
                    TODO_TRANSLATE("gimme 14D", TodoReason::SubjectToChange),
                // TODO do they also order more?
                .description = TODO_TRANSLATE(
                    "(unlocks a jukebox customers will pay to use)",
                    TodoReason::SubjectToChange),
                .onUnlock =
                    [](ConfigData& config, IsProgressionManager& ipm) {
                        // TODO what if instead of just money is also works to
                        // give you more time ot make drinks somehow? like they
                        // will order and leave the line to go to the jukebox
                        // and then when they get back the patience is renewed?
                        {
                            ipm.unlock_entity(EntityType::Jukebox);
                            config.store_to_spawn.push_back(
                                EntityType::Jukebox);
                            // Specifically not making it required forever
                        }
                    },
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // make sure we have at least 2 max orders
                    // because jukebox only works between orders
                    return config.get<int>(ConfigKey::MaxNumOrders) >= 2;
                }};
            break;
            // TODO is this too similar to mocktails?
        case UpgradeClass::CantEvenTell:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = TODO_TRANSLATE("Cant Even Tell",
                                       TodoReason::SubjectToChange),
                // TODO make upgrade icon for
                .icon_name = "upgrade_default",
                .flavor_text =
                    TODO_TRANSLATE("uhhh...gimme....ya...know...whatever",
                                   TodoReason::SubjectToChange),
                // TODO do they also order more?
                .description = TODO_TRANSLATE("(the more drinks a customer has "
                                              "the less the recipe matters)",
                                              TodoReason::SubjectToChange),
                .onUnlock = [](ConfigData&, IsProgressionManager&) {},
                .meetsPrereqs = [](const ConfigData& config,
                                   const IsProgressionManager&) -> bool {
                    // make sure we have at least 4 max orders
                    // because it doesnt matter if people cant actually have
                    // more drinks
                    return config.get<int>(ConfigKey::MaxNumOrders) >= 4 &&
                           !config.has_upgrade_unlocked(
                               UpgradeClass::Mocktails);
                }};
            break;
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }

    return std::shared_ptr<UpgradeImpl>(ptr);
}
