#pragma once

#include "../components/has_fishing_game.h"
#include "../engine/assert.h"

namespace tests {

inline void test_has_fishing_game() {
    using Phase = HasFishingGame::Phase;

    HasFishingGame game;
    VALIDATE(game.phase() == Phase::NotStarted,
             "new fishing game should not be started");
    VALIDATE(!game.show_progress_bar(),
             "progress bar should hide before start");

    game.go(0.01f);
    VALIDATE(game.phase() == Phase::Running,
             "started fishing game should be running");
    VALIDATE(game.show_progress_bar(),
             "progress bar should show while running");

    game.pass_time(1.f);
    VALIDATE(game.phase() == Phase::Scored,
             "expired countdown should score the game");
    VALIDATE(game.is_phase(Phase::Scored),
             "is_phase should report scored");
    VALIDATE(!game.show_progress_bar(),
             "progress bar should hide once scored");
}

}  // namespace tests
