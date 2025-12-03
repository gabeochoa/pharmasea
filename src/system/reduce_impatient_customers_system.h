#include "../ah.h"
#include "../components/has_patience.h"
#include "../engine/statemanager.h"
#include "../log/log.h"

namespace system_manager {
struct ReduceImpatientCustomersSystem : public afterhours::System<HasPatience> {
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

    virtual void for_each_with(Entity&, HasPatience& patience,
                               float dt) override {
        if (!patience.should_pass_time()) return;

        patience.pass_time(dt);

        // TODO actually do something when they get mad
        if (patience.pct() <= 0) {
            patience.reset();
            log_warn("You wont like me when im angry");
        }
    }
};
}  // namespace system_manager