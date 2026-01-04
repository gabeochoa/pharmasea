#pragma once

#include "base_component.h"

#include <optional>

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

    // Staged transition target (for two-phase "decide then commit" updates).
    // When set, systems should avoid mutating the authoritative `state` directly
    // and instead allow a commit step to apply this value.
    std::optional<State> next_state;

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
    IsAIControlled& enable_ability(AbilityFlags flag) {
        abilities |= static_cast<uint32_t>(flag);
        return *this;
    }
    IsAIControlled& disable_ability(AbilityFlags flag) {
        abilities &= ~static_cast<uint32_t>(flag);
        return *this;
    }
    IsAIControlled& set_ability_state(AbilityFlags flag, bool enabled) {
        return enabled ? enable_ability(flag) : disable_ability(flag);
    }

    // Optional: preserve current “wandering is a pause” behavior.
    // TODO: Revisit this. We currently use resume_state as a "return to previous
    // task" slot when Wander is used as a temporary pause; ensure this is still
    // the right abstraction once AI decisions/interrupts are formalized.
    State resume_state = State::Wander;

    // Setup helper (for makers) so call sites can chain configuration.
    IsAIControlled& set_initial_state(State s) {
        state = s;
        return *this;
    }

    // Immediate setter (authoritative). Prefer `set_state(...)` for staged
    // transitions once the commit system is in place.
    IsAIControlled& set_state_immediately(State s) {
        state = s;
        return *this;
    }

    // Staged setter (requested transition). This writes `next_state` and is
    // intended to be committed by `AICommitNextStateSystem`.
    //
    // Note: does not override an existing pending next_state.
    IsAIControlled& set_state(State s) {
        (void) set_next_state(s);
        return *this;
    }

    [[nodiscard]] bool has_next_state() const { return next_state.has_value(); }

    // Returns true if the next state was set. Returns false if a next state was
    // already pending.
    bool set_next_state(State s) {
        if (next_state.has_value()) return false;
        next_state = s;
        return true;
    }

    // Always set the next state (used for "override" transitions like bathroom).
    void force_next_state(State s) { next_state = s; }

    void clear_next_state() { next_state.reset(); }

    IsAIControlled& set_resume_state(State s) {
        resume_state = s;
        return *this;
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.state,                       //
            self.next_state,                  //
            self.abilities,                   //
            self.resume_state                 //
        );
    }
};

