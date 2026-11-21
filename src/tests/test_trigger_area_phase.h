#pragma once

#include "../components/is_trigger_area.h"
#include "../engine/assert.h"

namespace tests {

inline void test_trigger_area_phase() {
    using Phase = IsTriggerArea::Phase;

    IsTriggerArea area(IsTriggerArea::Type::Lobby_PlayGame);
    VALIDATE(area.phase() == Phase::Idle,
             "new trigger area should be idle");

    area.update_progress_max(2.f);
    area.increase_progress(1.f);
    VALIDATE(area.phase() == Phase::Filling,
             "partial progress should be filling");

    area.increase_progress(1.f);
    VALIDATE(area.phase() == Phase::Full,
             "complete progress should be full");
    VALIDATE(area.is_phase(Phase::Full),
             "is_phase should agree with phase()");

    IsTriggerArea cooling(IsTriggerArea::Type::Lobby_PlayGame);
    cooling.update_cooldown_max(3.f);
    cooling.reset_cooldown();
    VALIDATE(cooling.phase() == Phase::Cooldown,
             "reset cooldown should read as cooldown");
}

}  // namespace tests
