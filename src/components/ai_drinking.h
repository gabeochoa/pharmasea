#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "ai_component.h"
#include "base_component.h"

struct AIDrinking : public AIComponent {
    struct AIDrinkingTarget : AITarget {
        explicit AIDrinkingTarget(const ResetFn& resetFn) : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& customer) override {
            // TODO :MAKE_DURING_FIND: need a way to clean this up on unset
            auto& entity = EntityHelper::createEntity();

            vec2 position = vec2{0, 0};

            int attempts_rem = 100;
            while (attempts_rem > 0) {
                attempts_rem--;

                vec2 new_pos = vec2{
                    randfIn(-10, 10),
                    randfIn(-10, 10),
                };

                const auto walkable = EntityHelper::isWalkable(new_pos);

                if (walkable) {
                    position = customer.get<Transform>().as2() + new_pos;
                    break;
                }
            }

            // TODO in debug mode add renderer for ai target location?
            convert_to_type(EntityType::AITargetLocation, entity, position);
            return entity;
        }
    } target;

    AITakesTime timer;

    AIDrinking() : target(std::bind(&AIComponent::reset, this)) {}

    virtual ~AIDrinking() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
        s.object(target);
    }
};
