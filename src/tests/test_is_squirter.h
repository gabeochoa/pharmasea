#pragma once

#include "../components/is_squirter.h"
#include "../engine/assert.h"

namespace tests {

inline void test_is_squirter() {
    using Phase = IsSquirter::Phase;

    IsSquirter squirter;
    VALIDATE(squirter.phase() == Phase::Idle,
             "new squirter without an item should be idle");

    squirter.update(42, vec3{0.f, 0.f, 0.f});
    VALIDATE(squirter.phase() == Phase::Armed,
             "squirter holding a matched item should be armed");
    VALIDATE(squirter.is_phase(Phase::Armed),
             "is_phase should agree with phase()");

    VALIDATE(!squirter.pass_time(0.5f),
             "pass_time should not finish before the countdown");
    VALIDATE(squirter.phase() == Phase::Pouring,
             "squirter mid-countdown should be pouring");

    VALIDATE(squirter.pass_time(1.f),
             "pass_time should finish after the countdown");
    VALIDATE(squirter.phase() == Phase::Idle,
             "finished squirter should reset to idle");
}

}  // namespace tests
