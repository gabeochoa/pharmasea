#pragma once

#include "../../../ah.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_pnumatic_pipe.h"
#include "../../../engine/statemanager.h"
#include "../../../entities/entity_helper.h"
#include "../../../entities/entity_query.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct ProcessPnumaticPipeMovementSystem
    : public afterhours::System<
          IsPnumaticPipe, afterhours::tags::All<EntityType::PnumaticPipe>> {
    virtual ~ProcessPnumaticPipeMovementSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, IsPnumaticPipe& ipp,
                               float) override {
        if (!ipp.has_pair()) return;

        OptEntity paired_pipe = EntityQuery()
                                    .whereID(ipp.paired.id)
                                    .whereHasComponent<IsPnumaticPipe>()
                                    .gen_first();

        if (!paired_pipe) return;

        const IsPnumaticPipe& other_ipp = paired_pipe->get<IsPnumaticPipe>();

        // if they are both recieving or both sending, do nothing
        if (ipp.recieving == other_ipp.recieving) return;

        // we are recieving, they are sending
        if (ipp.recieving) {
            // we are the reciever, we should take the item
            if (paired_pipe->is_missing<CanHoldItem>()) return;
            CanHoldItem& other_chi = paired_pipe->get<CanHoldItem>();

            if (other_chi.empty()) return;

            if (entity.is_missing<CanHoldItem>()) return;
            CanHoldItem& our_chi = entity.get<CanHoldItem>();

            if (our_chi.is_holding_item()) return;

            OptEntity other_item_opt = other_chi.const_item();
            if (!other_item_opt) {
                other_chi.update(nullptr, entity_id::INVALID);
                return;
            }
            Entity& other_item = other_item_opt.asE();

            // can we hold it?
            if (!our_chi.can_hold(other_item, RespectFilter::ReqOnly)) return;

            // take it
            our_chi.update(other_item, entity.id);
            other_chi.update(nullptr, entity_id::INVALID);

            // we are done recieving
            entity.get<IsPnumaticPipe>().recieving = false;
        }
    }
};

}  // namespace system_manager