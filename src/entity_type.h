
#pragma once

#include <bitset>
#include <iostream>

#include "vendor_include.h"

enum struct EntityType {
    Unknown,
    x,
    y,
    z,

    RemotePlayer,
    Player,
    Customer,
    Table,
    CharacterSwitcher,
    MapRandomizer,
    Wall,
    Conveyer,
    Grabber,
    Register,
    SingleAlcohol,
    AlcoholCabinet,
    FruitBasket,
    TriggerArea,
    FloorMarker,
    CustomerSpawner,
    Sophie,
    Blender,
    SodaMachine,
    Cupboard,
    PitcherCupboard,
    Squirter,
    FilteredGrabber,
    PnumaticPipe,
    Vomit,
    MopHolder,
    FastForward,
    MopBuddyHolder,
    MopBuddy,
    SimpleSyrupHolder,
    IceMachine,
    Face,
    DraftTap,
    Toilet,
    Guitar,
    ChampagneHolder,  // TODO i wish there was an easier way to have this be
                      // alcohol without adding it to the cycler
    Jukebox,
    AITargetLocation,
    InteractiveSettingChanger,
    Door,
    SodaFountain,

    //
    // Items
    //

    SodaSpout,
    // TODO i think there could be a way to merge pitcher and drink but for
    // ease, lets start with separate ents
    Pitcher,
    Drink,
    Champagne,
    Alcohol,
    Fruit,
    FruitJuice,
    SimpleSyrup,
    Mop,
    HandTruck,

    // NOTE: Every time you add something above, add a 1 on the right of the
    // number below
    Trash,
    // Dont add anything below this, we want trash to be either first or second
    // first so its easier to track in non-destructive
};

using EntityTypeSet = std::bitset<magic_enum::enum_count<EntityType>()>;

constexpr EntityTypeSet create_non_destructive() {
#ifdef __APPLE__
    return EntityTypeSet()
        .set()  //
        .reset(static_cast<int>(EntityType::Trash))
        //
        ;
#else
    // TODO :INFRA: MSVC doesnt have enough constexpr constructors for bitset
    // https://learn.microsoft.com/en-us/cpp/standard-library/bitset-class?view=msvc-170#bitset
    // generate number through: https://godbolt.org/z/ef7sTsWb6
    return 0b0111111111111111111111111111111111111111111111111111;
#endif
}

const static EntityTypeSet ETS_NON_DESTRUCTIVE = create_non_destructive();

inline std::string_view str(const EntityType& type) {
    return magic_enum::enum_name(type);
}

inline std::ostream& operator<<(std::ostream& os, const EntityType& type) {
    os << "EntityType: " << magic_enum::enum_name(type);
    return os;
}

inline constexpr int get_price_for_entity_type(EntityType type) {
    switch (type) {
        // TODO this one should be based on average customer spend
        // customer count * avg unlocked drink price * 0.5
        case EntityType::Guitar:   // TODO add support for dynamic prices
        case EntityType::Jukebox:  // TODO decide based off how much money it
                                   // makes
            return 1;
        case EntityType::PitcherCupboard:
        case EntityType::Cupboard:
        case EntityType::Trash:
        case EntityType::Table:
        case EntityType::Register:
            return 5;
        case EntityType::ChampagneHolder:
        case EntityType::DraftTap:
        case EntityType::SodaMachine:
        // TODO index creation not supported yet
        // case EntityType::SingleAlcohol:
        case EntityType::Blender:
        case EntityType::IceMachine:
        case EntityType::SimpleSyrupHolder:
        case EntityType::MopHolder:
            return 10;
        case EntityType::AlcoholCabinet:
        case EntityType::FruitBasket:
        case EntityType::Conveyer:
        case EntityType::Toilet:
            return 50;
        case EntityType::Grabber:
        case EntityType::MopBuddyHolder:
            return 100;
        case EntityType::Squirter:
        case EntityType::FilteredGrabber:
        case EntityType::PnumaticPipe:
        case EntityType::HandTruck:
        case EntityType::SodaFountain:
            return 200;
            // Non buyables
        case EntityType::FastForward:
            // TODO see todo above...
        case EntityType::SingleAlcohol:
            //
        case EntityType::MopBuddy:
        case EntityType::SodaSpout:
        case EntityType::Drink:
        case EntityType::Alcohol:
        case EntityType::Fruit:
        case EntityType::FruitJuice:
        case EntityType::SimpleSyrup:
        case EntityType::Vomit:
        case EntityType::Mop:
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        case EntityType::CustomerSpawner:
        case EntityType::Sophie:
        case EntityType::RemotePlayer:
        case EntityType::Player:
        case EntityType::Customer:
        case EntityType::CharacterSwitcher:
        case EntityType::MapRandomizer:
        case EntityType::Wall:
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
        case EntityType::Face:
        case EntityType::Pitcher:
        case EntityType::Champagne:
        case EntityType::AITargetLocation:
        case EntityType::InteractiveSettingChanger:
        case EntityType::Door:
            // log_warn("You should probably not need the price for this {}",
            // magic_enum::enum_name<EntityType>(type));
            return -1;
    }
    return 0;
}

enum struct StoreEligibilityType {
    Never,
    OnStart,
    TimeBased,
    IngredientBased,
    OnUpgrade,
};

inline StoreEligibilityType get_store_eligibility(EntityType etype) {
    switch (etype) {
        case EntityType::ChampagneHolder:
        case EntityType::PitcherCupboard:
        case EntityType::Guitar:
        case EntityType::Toilet:
        case EntityType::Jukebox:
            return StoreEligibilityType::OnUpgrade;
        case EntityType::Table:
        case EntityType::Register:
        case EntityType::SingleAlcohol:
        case EntityType::Cupboard:
        case EntityType::SodaMachine:
        case EntityType::Trash:
        case EntityType::HandTruck:
        case EntityType::SodaFountain:
            return StoreEligibilityType::OnStart;
        case EntityType::Conveyer:
        case EntityType::DraftTap:
        case EntityType::Grabber:
        case EntityType::AlcoholCabinet:
        case EntityType::Blender:
        case EntityType::Squirter:
        case EntityType::FilteredGrabber:
        case EntityType::PnumaticPipe:
        case EntityType::MopHolder:
        case EntityType::MopBuddyHolder:
            // TODO Right now we unlock these at the start of game
            //      but we probably should build a system to
            //      configure day count or upgrade count or something
            return StoreEligibilityType::TimeBased;
        case EntityType::FruitBasket:
        case EntityType::SimpleSyrupHolder:
        case EntityType::IceMachine:
        case EntityType::SimpleSyrup:
            return StoreEligibilityType::IngredientBased;
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
        case EntityType::RemotePlayer:
        case EntityType::Player:
        case EntityType::Customer:
        case EntityType::CharacterSwitcher:
        case EntityType::MapRandomizer:
        case EntityType::Wall:
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        case EntityType::CustomerSpawner:
        case EntityType::Sophie:
        case EntityType::Vomit:
        case EntityType::FastForward:
        case EntityType::MopBuddy:
        case EntityType::Mop:
        case EntityType::Face:
        case EntityType::Drink:
        case EntityType::Alcohol:
        case EntityType::Fruit:
        case EntityType::FruitJuice:
        case EntityType::SodaSpout:
        case EntityType::Pitcher:
        case EntityType::Champagne:
        case EntityType::AITargetLocation:
        case EntityType::InteractiveSettingChanger:
        case EntityType::Door:
            return StoreEligibilityType::Never;
    }
    return StoreEligibilityType::Never;
}
