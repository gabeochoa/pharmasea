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
    bool isopen = false;
    void on_complete() {
        isopen = !isopen;
        // GameState::s_toggle_planning();
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

        s.value1b(isopen);

        s.value4b(type);
        s.value4b(currentRoundTime);
        s.value4b(totalRoundTime);
    }
};
