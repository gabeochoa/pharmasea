
#include "entity.h"

#include "components/debug_name.h"

Entity::~Entity() {
    for (auto itr = componentArray.begin(); itr != componentArray.end();
         itr++) {
        if (itr->second) delete (itr->second);
    }
    componentArray.clear();
}

void Entity::errorIfMissingDebugName() const {
    if (this->is_missing<DebugName>()) {
        log_error(
            "This entity is missing debugname which will cause issues for "
            "if the get<> is missing");
    }
}

const std::string_view Entity::name() const {
    return this->get<DebugName>().name();
}

bool check_type(const Entity& entity, EntityType type) {
    return entity.get<DebugName>().is_type(type);
}
bool check_if_drink(const Entity& entity) {
    switch (entity.get<DebugName>().get_type()) {
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
            return false;
        case EntityType::Pitcher:
        case EntityType::Drink:
            return true;
    }
    return false;
}
