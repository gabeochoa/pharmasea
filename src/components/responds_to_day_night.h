

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
        if (onDayStartedFn && parent) onDayStartedFn(*parent);
    }
    void call_night_started() {
        if (onNightStartedFn && parent) onNightStartedFn(*parent);
    }

    void call_day_ended() {
        if (onDayEndedFn && parent) onDayEndedFn(*parent);
    }
    void call_night_ended() {
        if (onNightEndedFn && parent) onNightEndedFn(*parent);
    }

   private:
    OnDayStartedFn onDayStartedFn = nullptr;
    OnDayEndedFn onDayEndedFn = nullptr;

    OnNightStartedFn onNightStartedFn = nullptr;
    OnNightEndedFn onNightEndedFn = nullptr;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
