
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
    }

    void start_night() {
        is_day = false;
        current_length += night_length;
        needs_to_process_change = true;
    }

    HasDayNightTimer(float round_length)
        : day_count(0),
          day_length(round_length),
          night_length(round_length),
          current_length(round_length) {}

    HasDayNightTimer() : HasDayNightTimer(10.f) {}

   private:
    int day_count;

    float day_length;
    float night_length;

    // how much time remaining in the current state
    float current_length;
    bool is_day = true;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(day_length);
        s.value4b(night_length);
        s.value4b(current_length);
        s.value1b(is_day);
    }
};
