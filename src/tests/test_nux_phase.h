#pragma once

#include "../components/is_nux_manager.h"
#include "../engine/assert.h"

namespace tests {

inline void test_nux_phase() {
    using NuxPhase = IsNux::NuxPhase;

    IsNux nux;
    VALIDATE(nux.phase() == NuxPhase::Dormant,
             "new nux without lambdas should be dormant");

    nux.set_eligibility_fn([](const IsNux&) { return true; });
    VALIDATE(nux.phase() == NuxPhase::Eligible,
             "nux with a passing trigger should be eligible");

    nux.set_completion_fn([](const IsNux&) { return true; });
    VALIDATE(nux.phase() == NuxPhase::Complete,
             "nux with a passing completion should be complete");

    nux.is_active = true;
    VALIDATE(nux.phase() == NuxPhase::Active,
             "active nux should report active above all");
    VALIDATE(nux.is_phase(NuxPhase::Active),
             "is_phase should agree with phase()");
}

}  // namespace tests
