#pragma once

#include "../components/has_work.h"
#include "../engine/assert.h"

namespace tests {

inline void test_has_work() {
    using Phase = HasWork::Phase;

    HasWork work;
    VALIDATE(work.phase() == Phase::Idle,
             "new work should start idle");
    VALIDATE(work.is_phase(Phase::Idle),
             "is_phase should agree with phase()");

    work.increase_pct(0.5f);
    VALIDATE(work.phase() == Phase::Working,
             "partial progress should be working");

    work.update_pct(1.f);
    VALIDATE(work.phase() == Phase::Complete,
             "full progress should be complete");
    VALIDATE(work.is_phase(Phase::Complete),
             "is_phase should report complete");

    work.reset_pct();
    VALIDATE(work.phase() == Phase::Idle,
             "reset work should be idle again");
}

}  // namespace tests
