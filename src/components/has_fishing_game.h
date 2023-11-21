
#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct HasFishingGame : public BaseComponent {
    HasFishingGame() {}

    [[nodiscard]] bool has_score() const { return score != -1; }
    [[nodiscard]] bool show_progress_bar() const { return started; }
    [[nodiscard]] float get_score() const { return score; }
    [[nodiscard]] float pct() const { return progress; }

    void pass_time(float dt) {
        if (!started || has_score()) return;
        countdown -= dt;

        log_info("countdown {:2f}", countdown);
        if (countdown <= 0.f) {
            // TODO figure out scoring
            score = abs(progress - 0.5f);
        }
    }

    void go(float dt) {
        if (has_score()) return;

        if (!started) started = true;
        countdown = countdownReset;
        bounce(dt);
        log_info("dt {:2f} dir {} progress: {:2f}", dt, direction, progress);
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
    void increment(float dt) { progress += direction * dt; }

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

        s.value4b(score);
        s.value4b(progress);
        s.value4b(countdown);
    }
};
