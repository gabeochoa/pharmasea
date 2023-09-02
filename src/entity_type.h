
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
    MedicineCabinet,
    PillDispenser,
    TriggerArea,
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
    MopBuddy,

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

    // NOTE: Every time you add something above, add a 1 to the number below on
    Trash,

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
    return 0b101111111111111111111111111111111111;
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
