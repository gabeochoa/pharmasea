#pragma once

#include "../components/is_progression_manager.h"
#include "../engine/assert.h"

namespace tests {

inline void test_progression_phase() {
    using ProgressionPhase = IsProgressionManager::ProgressionPhase;

    IsProgressionManager ipm;
    VALIDATE(ipm.phase() == ProgressionPhase::AwaitingOptions,
             "new progression manager should await options");
    VALIDATE(ipm.is_phase(ProgressionPhase::AwaitingOptions),
             "is_phase should agree with phase()");

    ipm.collectedOptions = true;
    VALIDATE(ipm.phase() == ProgressionPhase::OptionsReady,
             "collected options should be options ready");

    const std::vector<UpgradeType> saved_rounds = upgrade_rounds;
    upgrade_rounds = {UpgradeType::None, UpgradeType::Drink};
    ipm.upgrade_index = 0;
    ipm.advance_cycle();
    VALIDATE(ipm.upgrade_index == 1,
             "advance_cycle should move to the next round");
    VALIDATE(ipm.phase() == ProgressionPhase::AwaitingOptions,
             "advance_cycle should reset to awaiting options");
    ipm.advance_cycle();
    VALIDATE(ipm.upgrade_index == 0,
             "advance_cycle should wrap around the rounds");
    upgrade_rounds = saved_rounds;
}

}  // namespace tests
