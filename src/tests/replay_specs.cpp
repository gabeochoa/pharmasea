#include "../engine/simulated_input/replay_validation.h"

#include "../components/has_day_night_timer.h"
#include "../engine/assert.h"
#include "../entity_query.h"
#include "../entity_type.h"

namespace {

struct RegisterReplaySpecsOnce {
    RegisterReplaySpecsOnce() {
        replay_validation::register_spec(replay_validation::Spec{
            .name = "completed_first_day",
            .on_end =
                [](replay_validation::Context&) {
                    OptEntity sophie =
                        EntityQuery().whereType(EntityType::Sophie).gen_first();
                    VALIDATE(sophie.valid(),
                             "replay validation requires Sophie to exist");
                    const int day_count =
                        sophie->get<HasDayNightTimer>().days_passed();

                    // For the "complete first day" replay we expect to be in
                    // Day 1 by the end (HasDayNightTimer increments at day
                    // start).
                    M_TEST_EQ(day_count, 1,
                              "expected day_count == 1 by end of replay");
                },
        });
    }
};

static RegisterReplaySpecsOnce gRegisterReplaySpecsOnce;

}  // namespace

