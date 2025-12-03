

#include "../ah.h"

namespace system_manager {
struct ResetEmptyWorkFurnitureSystem
    : public afterhours::System<HasWork, CanHoldItem> {
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

    void for_each_with(Entity&, HasWork& hasWork, CanHoldItem& chi,
                       float) override {
        if (!hasWork.should_reset_on_empty()) return;
        if (chi.empty()) {
            hasWork.reset_pct();
            return;
        }

        // if its not empty, we have to see if its an item that can be
        // worked
    }
};
}  // namespace system_manager