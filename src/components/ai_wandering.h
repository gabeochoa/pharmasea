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
                    position = customer.get<Transform>().as2() + new_pos;
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

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                    //
            static_cast<AIComponent&>(self), //
            self.target,                    //
            self.timer,                     //
            self.next_job                   //
        );
    }
};
