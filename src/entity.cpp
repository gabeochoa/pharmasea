
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
