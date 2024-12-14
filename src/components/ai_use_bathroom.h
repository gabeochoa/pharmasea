
#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "../job.h"
#include "ai_component.h"
#include "base_component.h"
#include "is_toilet.h"

struct AIUseBathroom : public AIComponent {
    struct AIUseBathroomTarget : AITarget {
        explicit AIUseBathroomTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& ai_entity) override {
            return EntityQuery()
                .whereHasComponent<IsToilet>()
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full toilets
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                .whereCanPathfindTo(ai_entity.get<Transform>().as2())
                // Find the one with the least people on it
                .orderByLambda([](const Entity& r1, const Entity& r2) {
                    const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
                    int rpos1 = hwq1.get_next_pos();
                    const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
                    int rpos2 = hwq2.get_next_pos();
                    return rpos1 < rpos2;
                })
                .gen_first();
        }
    } target;

    AILineWait line_wait;
    AITakesTime timer;
    AITakesTime floor_timer;

    AIUseBathroom()
        : target(std::bind(&AIComponent::reset, this)),
          line_wait(std::bind(&AIComponent::reset, this)) {}
    virtual ~AIUseBathroom() {}

    JobType next_job;

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<AIComponent>(this), target, floor_timer);
    }
};

CEREAL_REGISTER_TYPE(AIUseBathroom);
