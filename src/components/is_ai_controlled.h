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

    // AI-only capability flags (what states/behaviors this controller is allowed
    // to enter). This intentionally lives on the AI controller so players don't
    // accidentally "inherit" AI-only components.
    enum AbilityFlags : uint32_t {
        AbilityNone = 0,
        AbilityCleanVomit = 1u << 0,
        AbilityUseBathroom = 1u << 1,
        AbilityPlayJukebox = 1u << 2,
    };
    uint32_t abilities = AbilityNone;

    [[nodiscard]] bool has_ability(AbilityFlags flag) const {
        return (abilities & static_cast<uint32_t>(flag)) != 0u;
    }
    void enable_ability(AbilityFlags flag) {
        abilities |= static_cast<uint32_t>(flag);
    }
    void disable_ability(AbilityFlags flag) {
        abilities &= ~static_cast<uint32_t>(flag);
    }

    // Optional: preserve current “wandering is a pause” behavior.
    State resume_state = State::Wander;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.state,                       //
            self.abilities,                   //
            self.resume_state                 //
        );
    }
};

