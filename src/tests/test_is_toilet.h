#pragma once

#include "../components/is_toilet.h"
#include "../engine/assert.h"

namespace tests {

inline void test_is_toilet() {
    using State = IsToilet::State;

    IsToilet toilet;
    VALIDATE(toilet.state == State::Available,
             "new toilet should be available");
    VALIDATE(toilet.available(), "new toilet should report available");

    toilet.start_use(42);
    VALIDATE(toilet.state == State::InUse,
             "start_use should mark the toilet in use");
    VALIDATE(toilet.occupied(), "in-use toilet should be occupied");
    VALIDATE(toilet.is_user(42), "toilet should remember its user");

    toilet.end_use();
    VALIDATE(toilet.state == State::Available,
             "end_use with uses left should free the toilet");
    VALIDATE(toilet.uses_remaining == 2,
             "end_use should consume one use");

    toilet.end_use();
    VALIDATE(toilet.uses_remaining == 2,
             "second end_use should be guarded and not decrement");

    toilet.start_use(43);
    toilet.end_use();
    toilet.start_use(44);
    toilet.end_use();
    VALIDATE(toilet.state == State::NeedsCleaning,
             "toilet with no uses left should need cleaning");

    toilet.reset();
    VALIDATE(toilet.state == State::Available,
             "reset should make the toilet available");
    VALIDATE(toilet.uses_remaining == toilet.total_uses,
             "reset should restore uses");
}

}  // namespace tests
