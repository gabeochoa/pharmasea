
#pragma once

#include "../engine/constexpr_containers.h"
#include "../engine/log.h"
#include "base_component.h"

struct HasFishingGame : public BaseComponent {
    struct Band {
        float score_percentile;
        float multiplier;
        int num_stars;
        std::string icon;
    };

    std::array<Band, 4> score_band = {{
        {
            .score_percentile = 0.05f,
            .multiplier = 2.f,
            .num_stars = 3,
            .icon = "star_filled",
        },
        {
            .score_percentile = 0.15f,
            .multiplier = 1.2f,
            .num_stars = 2,
            .icon = "star_filled",
        },
        {
            .score_percentile = 0.25f,
            .multiplier = 1.f,
            .num_stars = 1,
            .icon = "star_empty",
        },
        {
            .score_percentile = 1.f,
            .multiplier = 0.9f,
            .num_stars = 1,
            .icon = "star_sad",
        },
    }};

    HasFishingGame() {}

    [[nodiscard]] bool has_score() const { return score != -1; }
    [[nodiscard]] bool show_progress_bar() const { return started; }
    [[nodiscard]] float get_score() const { return score; }
    [[nodiscard]] float pct() const { return progress; }
    [[nodiscard]] float best() const { return best_location; }
    [[nodiscard]] int num_stars() const { return m_num_stars; }

    void pass_time(float dt) {
        if (!started || has_score()) return;
        countdown -= dt;

        if (countdown <= 0.f) {
            float amount = abs(progress - best_location);

            int index = first_matching<Band, 4>(
                score_band, [amount](const Band& band) -> bool {
                    return amount < band.score_percentile;
                });
            if (index == -1) {
                log_warn("could not find matching score");
                score = 1.f;
                return;
            }

            const Band& band = score_band[index];
            score = band.multiplier;
            m_num_stars = band.num_stars;

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

    int m_num_stars = 0;

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                started, direction, m_num_stars, best_location, score, progress,
                countdown);
    }
};

CEREAL_REGISTER_TYPE(HasFishingGame);
