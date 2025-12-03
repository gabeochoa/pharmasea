#pragma once

#include "../ah.h"

namespace system_manager {

// System for processing pneumatic pipe movement during in-round updates
struct ProcessPnumaticPipeMovementSystem
    : public afterhours::System<IsPnumaticPipe, CanHoldItem> {
    virtual ~ProcessPnumaticPipeMovementSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_nighttime();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, IsPnumaticPipe& ipp,
                               CanHoldItem& can_hold_item, float) override {
        const CanHoldItem& chi = const_cast<CanHoldItem&>(can_hold_item);

        if (chi.empty()) {
            ipp.item_id = -1;
            ipp.recieving = false;
            return;
        }

        int cur_id = chi.const_item().id;
        ipp.item_id = cur_id;
    }
};

}  // namespace system_manager
