#pragma once

#include <optional>

#include "../engine/log.h"
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
    };

    [[nodiscard]] State state_value() const { return state_; }

    [[nodiscard]] bool state_matches(State s) const { return state_ == s; }

    [[nodiscard]] bool state_doesnt_match(State s) const {
        return !state_matches(s);
    }

    [[nodiscard]] static constexpr bool is_legal(State from, State to) {
        if (from == to) return true;
        switch (from) {
            case State::Wander:
                return to == State::QueueForRegister ||
                       to == State::Bathroom || to == State::PlayJukebox ||
                       to == State::CleanVomit || to == State::Leave;
            case State::QueueForRegister:
                return to == State::AtRegisterWaitForDrink ||
                       to == State::Bathroom || to == State::Wander ||
                       to == State::Leave;
            case State::AtRegisterWaitForDrink:
                return to == State::Drinking || to == State::Bathroom ||
                       to == State::Leave;
            case State::Drinking:
                return to == State::Pay || to == State::QueueForRegister ||
                       to == State::PlayJukebox || to == State::Leave;
            case State::Bathroom:
                return to == State::Wander ||
                       to == State::QueueForRegister ||
                       to == State::AtRegisterWaitForDrink ||
                       to == State::Drinking || to == State::Pay ||
                       to == State::PlayJukebox ||
                       to == State::CleanVomit || to == State::Leave;
            case State::Pay:
                return to == State::Leave || to == State::Wander;
            case State::PlayJukebox:
                return to == State::QueueForRegister ||
                       to == State::Wander || to == State::Bathroom ||
                       to == State::Leave;
            case State::CleanVomit:
                return to == State::Wander || to == State::Bathroom ||
                       to == State::Leave;
            case State::Leave:
                return to == State::Wander;
        }
        return false;
    }

   private:
    State state_ = State::Wander;

   public:

    // Staged transition target (for two-phase "decide then commit" updates).
    // When set, systems should avoid mutating the authoritative `state`
    // directly and instead allow a commit step to apply this value.
    std::optional<State> next_state;

    // AI-only capability flags (what states/behaviors this controller is
    // allowed to enter). This intentionally lives on the AI controller so
    // players don't accidentally "inherit" AI-only components.
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
    // TODO: Revisit this. We currently use resume_state as a "return to
    // previous task" slot when Wander is used as a temporary pause; ensure this
    // is still the right abstraction once AI decisions/interrupts are
    // formalized.
    State resume_state = State::Wander;

    // Setup helper (for makers) so call sites can chain configuration.
    IsAIControlled& set_initial_state(State s) {
        state_ = s;
        return *this;
    }

    // Immediate setter (authoritative). Prefer `set_state(...)` for staged
    // transitions once the commit system is in place.
    IsAIControlled& set_state_immediately(State s) {
        if (!is_legal(state_, s)) {
            log_warn("IsAIControlled illegal transition {} -> {}",
                     magic_enum::enum_name(state_),
                     magic_enum::enum_name(s));
        }
        state_ = s;
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

    void clear_next_state() { next_state.reset(); }

    IsAIControlled& set_resume_state(State s) {
        resume_state = s;
        return *this;
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.state_,                        //
            self.next_state,                    //
            self.abilities,                     //
            self.resume_state                   //
        );
    }
};
