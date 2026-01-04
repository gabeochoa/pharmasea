
#include "entity.h"

#include "ah.h"
#include "entity_type.h"

#include <algorithm>
#include "magic_enum/magic_enum.hpp"

bool check_type(const Entity& entity, EntityType type) {
    return entity.hasTag(type);
}

bool check_if_drink(const Entity& entity) {
    EntityType type = get_entity_type(entity);
    switch (type) {
        case EntityType::Pitcher:
        case EntityType::Drink:
            return true;
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
        case EntityType::RemotePlayer:
        case EntityType::Player:
        case EntityType::Customer:
        case EntityType::Table:
        case EntityType::CharacterSwitcher:
        case EntityType::MapRandomizer:
        case EntityType::Wall:
        case EntityType::Conveyer:
        case EntityType::Grabber:
        case EntityType::Register:
        case EntityType::SingleAlcohol:
        case EntityType::AlcoholCabinet:
        case EntityType::FruitBasket:
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        case EntityType::CustomerSpawner:
        case EntityType::Sophie:
        case EntityType::Blender:
        case EntityType::SodaMachine:
        case EntityType::Cupboard:
        case EntityType::PitcherCupboard:
        case EntityType::Squirter:
        case EntityType::FilteredGrabber:
        case EntityType::PnumaticPipe:
        case EntityType::Vomit:
        case EntityType::MopHolder:
        case EntityType::FastForward:
        case EntityType::MopBuddyHolder:
        case EntityType::MopBuddy:
        case EntityType::SimpleSyrupHolder:
        case EntityType::IceMachine:
        case EntityType::Face:
        case EntityType::DraftTap:
        case EntityType::Toilet:
        case EntityType::Guitar:
        case EntityType::ChampagneHolder:
        case EntityType::Jukebox:
        case EntityType::AITargetLocation:
        case EntityType::InteractiveSettingChanger:
        case EntityType::Door:
        case EntityType::SodaFountain:
        case EntityType::SodaSpout:
        case EntityType::Champagne:
        case EntityType::Alcohol:
        case EntityType::Fruit:
        case EntityType::FruitJuice:
        case EntityType::SimpleSyrup:
        case EntityType::Mop:
        case EntityType::HandTruck:
        case EntityType::Trash:
            return false;
    }
    return false;
}

EntityType get_entity_type(const Entity& entity) {
    // We reserve the low tag IDs for EntityType and allow higher tag IDs for
    // additional runtime flags (AI transition tags, etc.).
    constexpr size_t kMaxTags = 64;
    const size_t kNumEntityTypeTags =
        std::min(static_cast<size_t>(magic_enum::enum_count<EntityType>()),
                 kMaxTags);

    int first_type_bit = -1;
    int type_bits_set = 0;
    for (size_t i = 0; i < kNumEntityTypeTags; ++i) {
        if (!entity.tags.test(i)) continue;
        type_bits_set++;
        if (first_type_bit == -1) first_type_bit = static_cast<int>(i);
        if (type_bits_set > 1) break;
    }

    if (type_bits_set == 0) {
        log_error("Entity has no EntityType tag set: {}", entity.tags.to_string());
        return EntityType::Unknown;
    }
    if (type_bits_set > 1) {
        log_error("Entity has multiple EntityType tags set: {}", entity.tags.to_string());
        return EntityType::Unknown;
    }

    return static_cast<EntityType>(first_type_bit);
}
