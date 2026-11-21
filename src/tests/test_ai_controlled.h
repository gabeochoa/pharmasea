#pragma once

#include "../components/is_ai_controlled.h"
#include "../engine/assert.h"

namespace tests {

inline void test_ai_controlled() {
    using State = IsAIControlled::State;

    VALIDATE(IsAIControlled::is_legal(State::Wander,
                                      State::QueueForRegister),
             "wander should queue for the register");
    VALIDATE(IsAIControlled::is_legal(State::Drinking, State::Pay),
             "drinking should proceed to pay");
    VALIDATE(IsAIControlled::is_legal(State::Pay, State::Leave),
             "paying should proceed to leave");
    VALIDATE(!IsAIControlled::is_legal(State::Wander, State::Pay),
             "wander should not jump straight to pay");
    VALIDATE(!IsAIControlled::is_legal(State::Leave,
                                       State::AtRegisterWaitForDrink),
             "leave should only return via wander");

    IsAIControlled ctrl;
    VALIDATE(ctrl.state_value() == State::Wander,
             "new controller should start wandering");
    VALIDATE(ctrl.state_matches(State::Wander),
             "state_matches should report the current state");
    VALIDATE(ctrl.state_doesnt_match(State::Pay),
             "state_matches should reject other states");

    ctrl.set_state_immediately(State::QueueForRegister);
    VALIDATE(ctrl.state_value() == State::QueueForRegister,
             "immediate setter should apply legal transitions");
}

}  // namespace tests
