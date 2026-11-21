#pragma once

#include "../components/bypass_automation_state.h"
#include "../engine/assert.h"

namespace tests {

inline void test_bypass_phase() {
    using BypassPhase = BypassAutomationState::BypassPhase;

    BypassAutomationState state;
    VALIDATE(state.phase() == BypassPhase::Idle,
             "new bypass state should be idle");

    state.configure(2, false);
    VALIDATE(state.phase() == BypassPhase::Startup,
             "configure should enter startup phase");

    state.advance(BypassPhase::Menu);
    state.advance(BypassPhase::Lobby);
    state.advance(BypassPhase::InGame);
    VALIDATE(state.is_phase(BypassPhase::InGame),
             "advance should track the current phase");

    state.mark_round_complete();
    VALIDATE(state.phase() == BypassPhase::InGame,
             "one remaining round should not finish the bypass");
    VALIDATE(!state.completed, "bypass should not be complete yet");

    state.mark_round_complete();
    VALIDATE(state.completed, "final round should complete the bypass");
    VALIDATE(state.phase() == BypassPhase::Done,
             "completed bypass should be done");
}

}  // namespace tests
