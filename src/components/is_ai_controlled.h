#pragma once

#include "base_component.h"

// Authoritative high-level AI controller state.
struct IsAIControlled : public BaseComponent {
    enum class State : int {
        Wander = 0,
        QueueForRegister,
        AtRegisterWaitForDrink,
        Drinking,
        Bathroom,
        Pay,
        PlayJukebox,
        CleanVomit,
        Leave,
    } state = State::Wander;

    // Optional: preserve current “wandering is a pause” behavior.
    State resume_state = State::Wander;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.state,                       //
            self.resume_state                 //
        );
    }
};

