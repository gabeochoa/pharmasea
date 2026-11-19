#include "../components/can_order_drink.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_bank.h"
#include "../components/is_customer.h"
#include "../engine/assert.h"
#include "../engine/simulated_input/replay_validation.h"
#include "../entities/entity_query.h"

namespace {

struct RegisterReplaySpecsOnce {
    RegisterReplaySpecsOnce() {
        replay_validation::register_spec(replay_validation::Spec{
            .name = "completed_first_day",
            .on_end =
                [](replay_validation::Context&) {
                    // Use EQ() so this works in both host/client contexts.
                    OptEntity e =
                        EQ().whereHasComponent<HasDayNightTimer>().gen_first();
                    VALIDATE(
                        e.valid(),
                        "replay validation requires HasDayNightTimer to exist");
                    const int day_count =
                        e->get<HasDayNightTimer>().days_passed();

                    // For the "complete first day" replay we expect to be in
                    // Day 1 by the end (HasDayNightTimer increments at day
                    // start).
                    M_TEST_EQ(day_count, 1,
                              "expected day_count == 1 by end of replay");
                },
        });

        replay_validation::register_spec(replay_validation::Spec{
            .name = "first_customer_served",
            .on_end =
                [](replay_validation::Context&) {
                    bool served = false;
                    RefEntities ents =
                        EQ().whereHasComponent<CanOrderDrink>().gen();
                    for (Entity& e : ents) {
                        if (e.get<CanOrderDrink>().num_drinks_drank() > 0)
                            served = true;
                    }
                    VALIDATE(served,
                             "expected at least one customer to have "
                             "finished a drink by end of replay");
                },
        });

        replay_validation::register_spec(replay_validation::Spec{
            .name = "first_purchase_placed",
            .on_end =
                [](replay_validation::Context&) {
                    OptEntity e = EQ().whereHasComponent<IsBank>().gen_first();
                    VALIDATE(e.valid(),
                             "replay validation requires IsBank to exist");
                    bool withdrew = false;
                    const IsBank& bank = e->get<IsBank>();
                    for (const IsBank::Transaction& t :
                         bank.get_transactions()) {
                        if (t.amount < 0) withdrew = true;
                    }
                    VALIDATE(withdrew,
                             "expected at least one bank withdrawal "
                             "(purchase or rent) by end of replay");
                },
        });

        replay_validation::register_spec(replay_validation::Spec{
            .name = "day1_cleanup_empty",
            .on_end =
                [](replay_validation::Context&) {
                    const int customers = static_cast<int>(
                        EQ().whereHasComponent<IsCustomer>().gen().size());
                    M_TEST_EQ(customers, 0,
                              "expected no customers left after day-1 "
                              "cleanup");
                },
        });
    }
};

static RegisterReplaySpecsOnce gRegisterReplaySpecsOnce;

}  // namespace
