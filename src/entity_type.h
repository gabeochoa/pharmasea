
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
    MedicineCabinet,
    PillDispenser,
    TriggerArea,
    FloorMarker,
    CustomerSpawner,
    Sophie,
    Blender,
    SodaMachine,
    Cupboard,
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

    //
    // Items
    //

    SodaSpout,
    Drink,
    Alcohol,
    Fruit,
    FruitJuice,
    SimpleSyrup,
    Mop,

    // NOTE: Every time you add something above, add a 1 on the right of the
    // number below
    Trash,
    // Dont add anything below this, we want trash to be either first or second
    // first so its easier to track in non-destructive

    MAX_ENTITY_TYPE
};

using EntityTypeSet = std::bitset<(int) EntityType::MAX_ENTITY_TYPE>;

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
    return 0b10111111111111111111111111111111111111111;
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
        case EntityType::Cupboard:
        case EntityType::Trash:
        case EntityType::Table:
        case EntityType::Register:
            return 10;
        case EntityType::SodaMachine:
        case EntityType::SingleAlcohol:
        case EntityType::Blender:
        case EntityType::IceMachine:
        case EntityType::SimpleSyrupHolder:
        case EntityType::MopHolder:
            return 20;
        case EntityType::MedicineCabinet:
        case EntityType::PillDispenser:
        case EntityType::Conveyer:
            return 100;
        case EntityType::Grabber:
        case EntityType::MopBuddyHolder:
            return 200;
        case EntityType::Squirter:
        case EntityType::FilteredGrabber:
        case EntityType::PnumaticPipe:
            return 500;
            // Non buyables
        case EntityType::FastForward:
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
        case EntityType::MAX_ENTITY_TYPE:
            // log_warn("You should probably not need the price for this {}",
            // magic_enum::enum_name<EntityType>(type));
            return -1;
    }
    return 0;
}
