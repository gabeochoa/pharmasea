#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AICleanVomit : public AIComponent {
    struct AICleanVomitTarget : AITarget {
        explicit AICleanVomitTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& entity) override {
            return EntityHelper::getClosestOfType(entity, EntityType::Vomit);
        }
    } target;

    AICleanVomit() : target(std::bind(&AIComponent::reset, this)) {}

    virtual ~AICleanVomit() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
        s.object(target);
    }
};
