
#pragma once

#include "external_include.h"
//
#include "drawing_util.h"
#include "statemanager.h"

struct Round {
    float currentRoundTime;
    float totalRoundTime;

    explicit Round(float t) {
        totalRoundTime = t;
        currentRoundTime = totalRoundTime;
    }

    void onUpdate(float dt) {
        if (!GameState::s_in_round()) return;

        if (currentRoundTime >= 0) currentRoundTime -= dt;
    }

    void onDraw() const {
        raylib::DrawPctFilledCircle({100, 100}, 50, BLACK, RED,
                                    (currentRoundTime / totalRoundTime));
    }

   private:
    friend bitsery::Access;
    // Note: this exists just for the serializer,
    Round() {
        totalRoundTime = 1.f;
        currentRoundTime = 1.f;
    }
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(currentRoundTime);
        s.value4b(totalRoundTime);
    }
};
