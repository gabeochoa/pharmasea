#pragma once

#include "../components/can_order_drink.h"
#include "../engine/assert.h"

namespace tests {

inline void test_can_order_drink() {
    using OrderState = CanOrderDrink::OrderState;
    using AIState = IsAIControlled::State;

    CanOrderDrink cod;
    VALIDATE(cod.order_state == OrderState::NeedsReset,
             "new customer should need reset");

    cod.reset_customer(1, Drink::coke);
    VALIDATE(cod.order_state == OrderState::Ordering,
             "reset customer should be ordering");

    cod.transition_to(OrderState::DrinkingNow);
    VALIDATE(cod.order_state == OrderState::DrinkingNow,
             "transition_to should move to drinking");

    cod.transition_to(OrderState::DoneDrinking);
    VALIDATE(cod.order_state == OrderState::DoneDrinking,
             "transition_to should move to done drinking");

    VALIDATE(order_matches_ai_state(OrderState::Ordering,
                                    AIState::QueueForRegister),
             "ordering should match queueing");
    VALIDATE(order_matches_ai_state(OrderState::DrinkingNow,
                                    AIState::Drinking),
             "drinking now should match drinking");
    VALIDATE(!order_matches_ai_state(OrderState::Ordering, AIState::Pay),
             "ordering should not match paying");
}

}  // namespace tests
