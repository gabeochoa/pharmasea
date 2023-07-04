#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "base_component.h"

struct HasTimer : public BaseComponent {
    // TODO using this cause i dont want to make a new component for each
    // renderer type
    enum Renderer {
        Round,
    } type;

    virtual ~HasTimer() {}

    HasTimer() : type(Round), currentRoundTime(10.f), totalRoundTime(10.f) {}

    explicit HasTimer(Renderer _type, float t) {
        type = _type;
        totalRoundTime = t;
        currentRoundTime = totalRoundTime;
    }

    auto& pass_time(float dt) {
        if (currentRoundTime >= 0) currentRoundTime -= dt;
        return *this;
    }

    [[nodiscard]] float pct() const {
        return currentRoundTime / totalRoundTime;
    }

    // TODO make these private
    float currentRoundTime;
    float totalRoundTime;

    // TODO move into its own component
    bool all_customers_out = false;
    void on_complete() {
        if (GameState::get().is(game::State::InRoundClosing)) {
            // For this one, we need to wait until everyone is done leaving
            if (!all_customers_out) return;
            GameState::get().set(game::State::Planning);
        }
        // For these all we have to do is go to the next state
        // and reset the timer
        else if (GameState::get().is(game::State::Planning)) {
            GameState::get().set(game::State::InRound);
        } else if (GameState::get().is(game::State::InRound)) {
            GameState::get().set(game::State::InRoundClosing);
        }
        currentRoundTime = totalRoundTime;
    }
    void reset_if_complete() {
        if (currentRoundTime <= 0.f) on_complete();
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        //
        s.value1b(all_customers_out);

        s.value4b(type);
        s.value4b(currentRoundTime);
        s.value4b(totalRoundTime);
    }
};
