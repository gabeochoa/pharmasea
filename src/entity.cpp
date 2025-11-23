
#include "entity.h"

#include "components/type.h"

bool check_type(const Entity& entity, EntityType type) {
    if (!entity.has<Type>()) {
        log_error(
            "check_type: entity {} does not have Type component. Checking for "
            "type: {}",
            entity.id, magic_enum::enum_name<EntityType>(type));
        // Will crash on next line to help identify the issue
    }
    return type == entity.get<Type>().type;
}

bool check_if_drink(const Entity& entity) {
    switch (entity.get<Type>().type) {
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
        case EntityType::SodaSpout:
        case EntityType::Alcohol:
        case EntityType::Fruit:
        case EntityType::FruitJuice:
        case EntityType::SimpleSyrup:
        case EntityType::Mop:
        case EntityType::Trash:
        case EntityType::PitcherCupboard:
        case EntityType::ChampagneHolder:
        case EntityType::Champagne:
        case EntityType::Jukebox:
        case EntityType::AITargetLocation:
        case EntityType::InteractiveSettingChanger:
        case EntityType::HandTruck:
        case EntityType::Door:
        case EntityType::SodaFountain:
            return false;
        case EntityType::Pitcher:
        case EntityType::Drink:
            return true;
    }
    return false;
}
