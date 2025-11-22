#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../components/transform.h"

namespace system_manager {

// Simple proof-of-concept system: Timer system that processes entities with
// HasDayNightTimer
struct TimerSystem : public afterhours::System<HasDayNightTimer> {
    virtual ~TimerSystem() = default;

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               [[maybe_unused]] HasDayNightTimer& timer,
                               [[maybe_unused]] float dt) override {
        // This is called automatically for all entities with HasDayNightTimer
        // component We can add the timer logic here later For now, this is just
        // a proof of concept
    }
};

}  // namespace system_manager
