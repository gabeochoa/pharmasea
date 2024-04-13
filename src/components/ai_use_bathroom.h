
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

        virtual OptEntity find_target(const Entity& entity) override {
            return EntityQuery()
                .whereHasComponent<IsToilet>()
                .whereLambda([](const Entity& entity) {
                    const IsToilet& toilet = entity.get<IsToilet>();
                    return toilet.available();
                })
                .orderByDist(entity.get<Transform>().as2())
                .gen_first();
        }
    } target;

    AIUseBathroom() : target(std::bind(&AIComponent::reset, this)) {}
    virtual ~AIUseBathroom() {}

    JobType next_job;

    float pissTime;
    void set_piss_time(float pt) { pissTime = pt; }
    [[nodiscard]] bool piss(float dt) {
        pissTime -= dt;
        return pissTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
