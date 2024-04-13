#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AIDrinking : public AIComponent {
    struct AIDrinkingTarget : AITarget {
        explicit AIDrinkingTarget(const ResetFn& resetFn) : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity&) override {
            // TODO :MAKE_DURING_FIND: need a way to clean this up on unset
            auto& entity = EntityHelper::createEntity();
            // TODO in debug mode add renderer for ai target location?
            convert_to_type(EntityType::AITargetLocation, entity,
                            // TODO choose a better place
                            vec2{0, 0});
            return entity;
        }
    } target;

    AIDrinking() : target(std::bind(&AIComponent::reset, this)) {}

    virtual ~AIDrinking() {}

    float drinkTime;
    void set_drink_time(float pt) { drinkTime = pt; }
    [[nodiscard]] bool drink(float dt) {
        drinkTime -= dt;
        return drinkTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
