#pragma once

#include "../../../ah.h"
#include "../../../components/is_pnumatic_pipe.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_query.h"

namespace system_manager {

struct ProcessPnumaticPipePairingSystem
    : public afterhours::System<IsPnumaticPipe> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp,
                               float) override {
        if (ipp.has_pair()) return;

        OptEntity other_pipe = EntityQuery()  //
                                   .whereNotMarkedForCleanup()
                                   .whereNotID(entity.id)
                                   .whereHasComponent<IsPnumaticPipe>()
                                   .whereLambda([](const Entity& pipe) {
                                       const IsPnumaticPipe& otherpp =
                                           pipe.get<IsPnumaticPipe>();
                                       // Find only the ones that dont have a
                                       // pair
                                       return !otherpp.has_pair();
                                   })
                                   .gen_first();

        if (other_pipe.has_value()) {
            IsPnumaticPipe& otherpp = other_pipe->get<IsPnumaticPipe>();
            otherpp.paired.set_id(entity.id);
            ipp.paired.set_id(other_pipe->id);
        }

        // still dont have a pair, we probably just have an odd number
        if (!ipp.has_pair()) return;
    }
};

}  // namespace system_manager