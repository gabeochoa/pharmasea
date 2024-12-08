
#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "ai_component.h"
#include "base_component.h"
#include "has_waiting_queue.h"

struct AIWaitInQueue : public AIComponent {
    struct AIWaitInQueueTarget : AITarget {
        explicit AIWaitInQueueTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& ai_entity) override {
            // TODO :DUPE: same as process_ai_paying
            return EntityQuery()
                .whereType(EntityType::Register)
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full registers
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                .whereCanPathfindTo(ai_entity.get<Transform>().as2())
                // Find the register with the least people on it
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

    AIWaitInQueue()
        : target(std::bind(&AIComponent::reset, this)),
          line_wait(std::bind(&AIComponent::reset, this)) {
        set_cooldown(0.1f);
    }

    virtual ~AIWaitInQueue() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
        s.object(target);
    }
};
