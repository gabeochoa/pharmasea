#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "ai_component.h"
#include "base_component.h"

struct AIWandering : public AIComponent {
    struct AIWanderingTarget : AITarget {
        explicit AIWanderingTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& customer) override {
            // TODO :MAKE_DURING_FIND: need a way to clean this up on unset
            auto& entity = EntityHelper::createEntity();

            vec2 position = vec2{0, 0};

            int attempts_rem = 100;
            while (attempts_rem > 0) {
                attempts_rem--;

                vec2 new_pos = RandomEngine::get().get_vec(-10, 10);

                const auto walkable = EntityHelper::isWalkable(new_pos);

                if (walkable) {
                    const vec2 base = customer.get<Transform>().as2();
                    position = vec2{base.x + new_pos.x, base.y + new_pos.y};
                    break;
                }
            }

            convert_to_type(EntityType::AITargetLocation, entity, position);
            return entity;
        }
    } target;

    AITakesTime timer;
    JobType next_job = JobType::Wandering;

    AIWandering() : target(std::bind(&AIComponent::reset, this)) {}

    virtual ~AIWandering() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
        s.object(target);
    }
};
