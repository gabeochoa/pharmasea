#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AICleanVomit : public AIComponent {
    AITarget target;

    AICleanVomit()
        : target(AITarget(
              [](const Entity& entity) -> OptEntity {
                  return EntityHelper::getClosestOfType(entity,
                                                        EntityType::Vomit);
              },
              std::bind(&AIComponent::reset, this))) {}

    virtual ~AICleanVomit() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
