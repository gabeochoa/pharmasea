
#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct HasFishingGame : public BaseComponent {
    HasFishingGame() {}

    [[nodiscard]] bool has_score() const { return score != -1; }
    [[nodiscard]] bool show_progress_bar() const { return started; }
    [[nodiscard]] float get_score() const { return score; }
    [[nodiscard]] float pct() const { return progress; }
    [[nodiscard]] float best() const { return best_location; }

    void pass_time(float dt) {
        if (!started || has_score()) return;
        countdown -= dt;

        if (countdown <= 0.f) {
            // TODO figure out scoring
            float amount = abs(progress - best_location);
            if (amount < 0.05f) {
                score = 2.f;
            } else if (amount < 0.15f) {
                score = 1.2f;
            } else if (amount < 0.25f) {
                score = 1.f;
            } else {
                score = 0.9f;
            }
            // TOOD add a UI element showing a x2 or whatever
            log_info("game ended got {}", score);
        }
    }

    void go(float dt) {
        if (has_score()) return;

        if (!started) started = true;
        countdown = countdownReset;
        bounce(dt);
    }

   private:
    [[nodiscard]] bool hit_end() const {
        if (progress <= 0.f || progress >= 1.f) return true;
        return false;
    }

    void bounce(float dt) {
        if (hit_end()) turn_around();
        increment(dt);
    }
    void turn_around() { direction *= -1; }
    void increment(float dt) { progress += direction * dt * 2.f; }

    float best_location = 0.5f;
    float score = -1;
    float progress = 0;
    int direction = 1;

    bool started = false;
    // note this is a little long still
    // but i havent tested over the network yet
    // so i imagine ping might kill this
    float countdown = 0.100f;
    float countdownReset = 0.100f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(started);

        s.value4b(direction);

        s.value4b(best_location);
        s.value4b(score);
        s.value4b(progress);
        s.value4b(countdown);
    }
};
