
#include "entity.h"

#include "engine/bitset_utils.h"
#include "entity_type.h"

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
    // Use bitset operations to find the first set bit (much faster than
    // iterating enum values)
    int first_bit = bitset_utils::get_first_enabled_bit(entity.tags);

    if (first_bit == -1) {
        // No tags set
        log_error("Entity has no tags set: {}", entity.tags.to_string());
        return EntityType::Unknown;
    }

    // Check if multiple tags are set by counting bits
    // If count > 1, return Unknown (entities should only have one EntityType
    // tag)
    if (entity.tags.count() > 1) {
        log_error("Entity has multiple tags set: {}", entity.tags.to_string());
        return EntityType::Unknown;
    }

    // Cast the bit index to EntityType
    return static_cast<EntityType>(first_bit);
}
