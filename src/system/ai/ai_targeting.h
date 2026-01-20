#pragma once

#include "../../components/has_waiting_queue.h"
#include "../../components/transform.h"
#include "../../entity.h"
#include "../../entity_query.h"

namespace system_manager::ai {

[[nodiscard]] inline OptEntity find_best_register_with_space(
    const Entity& ai_entity) {
    return EntityQuery()
        .whereType(EntityType::Register)
        .whereHasComponent<HasWaitingQueue>()
        .whereLambda([](const Entity& entity) {
            const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
            return !hwq.is_full();
        })
        .whereCanPathfindTo(ai_entity.get<Transform>().as2())
        // Find the register with the least people on it.
        .orderByLambda([](const Entity& r1, const Entity& r2) {
            const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
            int rpos1 = hwq1.get_next_pos();
            const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
            int rpos2 = hwq2.get_next_pos();
            return rpos1 < rpos2;
        })
        .gen_first();
}

}  // namespace system_manager::ai
