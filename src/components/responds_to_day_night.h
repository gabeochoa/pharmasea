

#pragma once

#include "base_component.h"

struct RespondsToDayNight : public BaseComponent {
    using OnDayStartedFn = std::function<void(Entity&)>;
    using OnDayEndedFn = std::function<void(Entity&)>;

    using OnNightStartedFn = std::function<void(Entity&)>;
    using OnNightEndedFn = std::function<void(Entity&)>;

    auto& registerOnDayStarted(const OnDayStartedFn& odsFn) {
        onDayStartedFn = odsFn;
        return *this;
    }

    auto& registerOnNightStarted(const OnNightStartedFn& odsFn) {
        onNightStartedFn = odsFn;
        return *this;
    }

    auto& registerOnDayEnded(const OnDayEndedFn& odsFn) {
        onDayEndedFn = odsFn;
        return *this;
    }

    auto& registerOnNightEnded(const OnNightEndedFn& odsFn) {
        onNightEndedFn = odsFn;
        return *this;
    }

    void call_day_started() {
        if (onDayStartedFn) onDayStartedFn(*parent);
    }
    void call_night_started() {
        if (onNightStartedFn) onNightStartedFn(*parent);
    }

    void call_day_ended() {
        if (onDayEndedFn) onDayEndedFn(*parent);
    }
    void call_night_ended() {
        if (onNightEndedFn) onNightEndedFn(*parent);
    }

   private:
    OnDayStartedFn onDayStartedFn = nullptr;
    OnDayEndedFn onDayEndedFn = nullptr;

    OnNightStartedFn onNightStartedFn = nullptr;
    OnNightEndedFn onNightEndedFn = nullptr;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};
CEREAL_REGISTER_TYPE(RespondsToDayNight);
