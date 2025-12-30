

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

    // NOTE: this component no longer stores an Entity* "parent".
    // Pass the owning entity explicitly to avoid pointer-based state.
    void call_day_started(Entity& owner) {
        if (onDayStartedFn) onDayStartedFn(owner);
    }
    void call_night_started(Entity& owner) {
        if (onNightStartedFn) onNightStartedFn(owner);
    }

    void call_day_ended(Entity& owner) {
        if (onDayEndedFn) onDayEndedFn(owner);
    }
    void call_night_ended(Entity& owner) {
        if (onNightEndedFn) onNightEndedFn(owner);
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
