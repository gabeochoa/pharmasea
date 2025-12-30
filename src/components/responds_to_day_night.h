

#pragma once

#include "../entity_helper.h"
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

    // Keep the existing API, but store the handle (id) instead of the pointer.
    void call_day_started() {
        if (!onDayStartedFn) return;
        OptEntity opt_parent = EntityHelper::getEntityForID(parent_id);
        if (!opt_parent) return;
        onDayStartedFn(opt_parent.asE());
    }
    void call_night_started() {
        if (!onNightStartedFn) return;
        OptEntity opt_parent = EntityHelper::getEntityForID(parent_id);
        if (!opt_parent) return;
        onNightStartedFn(opt_parent.asE());
    }

    void call_day_ended() {
        if (!onDayEndedFn) return;
        OptEntity opt_parent = EntityHelper::getEntityForID(parent_id);
        if (!opt_parent) return;
        onDayEndedFn(opt_parent.asE());
    }
    void call_night_ended() {
        if (!onNightEndedFn) return;
        OptEntity opt_parent = EntityHelper::getEntityForID(parent_id);
        if (!opt_parent) return;
        onNightEndedFn(opt_parent.asE());
    }

    auto& set_parent(Entity* p) {
        parent_id = p ? p->id : EntityID::INVALID;
        return *this;
    }

   private:
    OnDayStartedFn onDayStartedFn = nullptr;
    OnDayEndedFn onDayEndedFn = nullptr;

    OnNightStartedFn onNightStartedFn = nullptr;
    OnNightEndedFn onNightEndedFn = nullptr;

    EntityID parent_id = EntityID::INVALID;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(parent_id);
    }
};
