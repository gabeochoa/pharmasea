#pragma once

#include "../components/has_day_night_timer.h"
#include "../engine/assert.h"

namespace tests {

inline void test_has_day_night_timer() {
    using Phase = HasDayNightTimer::Phase;

    HasDayNightTimer timer(100.f);
    VALIDATE(timer.phase() == Phase::Planning,
             "new timer should start in planning phase");
    VALIDATE(timer.is_phase(Phase::Planning),
             "is_phase should agree with phase()");

    timer.open_bar();
    VALIDATE(timer.phase() == Phase::Transitioning,
             "open_bar should enter transitioning phase");
    VALIDATE(timer.needs_to_process_change(),
             "open_bar should flag a pending change");
    VALIDATE(timer.days_passed() == 1,
             "open_bar should advance the day counter");

    timer.finish_transition();
    VALIDATE(timer.phase() == Phase::InRound,
             "open timer without pending change should be in round");
    VALIDATE(!timer.needs_to_process_change(),
             "finished transition should clear the pending change");

    timer.close_bar();
    VALIDATE(timer.phase() == Phase::Transitioning,
             "close_bar should enter transitioning phase");

    timer.finish_transition();
    VALIDATE(timer.phase() == Phase::Planning,
             "closed timer without pending change should be planning");
}

}  // namespace tests
