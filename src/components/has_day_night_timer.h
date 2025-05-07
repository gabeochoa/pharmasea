
#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../vec_util.h"
#include "base_component.h"

struct HasDayNightTimer : public BaseComponent {
    // TODO for now since we cant use game state change
    // we need some way
    bool needs_to_process_change = false;

    [[nodiscard]] int days_passed() const { return day_count; }

    [[nodiscard]] bool is_daytime() const { return is_day; }
    [[nodiscard]] bool is_nighttime() const { return !is_daytime(); }
    [[nodiscard]] bool is_round_over() const { return current_length <= 0.f; }

    auto& pass_time(float dt) {
        if (current_length >= 0) current_length -= dt;
        return *this;
    }

    [[nodiscard]] float get_current_length() const { return current_length; }

    [[nodiscard]] float get_total_length() const {
        return is_daytime() ? day_length : night_length;
    }

    // slowly going down
    [[nodiscard]] float pct() const {
        return current_length / get_total_length();
    }

    void set_day_length(float trt) { day_length = trt; }
    void set_night_length(float trt) { night_length = trt; }

    void start_day() {
        is_day = true;
        day_count++;
        current_length += day_length;
        needs_to_process_change = true;

        days_until_rent_due--;
    }

    void start_night() {
        is_day = false;
        current_length += night_length;
        needs_to_process_change = true;
    }

    HasDayNightTimer(float round_length)
        : day_count(0),
          days_until_rent_due(5),
          day_length(round_length),
          night_length(round_length),
          current_length(round_length) {}

    HasDayNightTimer() : HasDayNightTimer(10.f) {}

    [[nodiscard]] int days_until() const { return days_until_rent_due; }
    [[nodiscard]] int rent_due() const { return amount_due; }
    void reset_rent_days() { days_until_rent_due = 5; }

    void update_amount_due(int new_amt) { amount_due = new_amt; }

   private:
    int day_count;
    int days_until_rent_due;
    // TODO - this should probably not live here but for now
    int amount_due = 75;

    float day_length;
    float night_length;

    // how much time remaining in the current state
    float current_length;
    bool is_day = true;

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                day_count, days_until_rent_due, amount_due, day_length,
                night_length, current_length, is_day);
    }
};

CEREAL_REGISTER_TYPE(HasDayNightTimer);
