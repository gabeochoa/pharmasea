

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

    void call_day_started() {
        if (!onDayStartedFn) return;
        Entity& parent = EntityHelper::getEnforcedEntityForID(parent_id);
        onDayStartedFn(parent);
    }
    void call_night_started() {
        if (!onNightStartedFn) return;
        Entity& parent = EntityHelper::getEnforcedEntityForID(parent_id);
        onNightStartedFn(parent);
    }

    void call_day_ended() {
        if (!onDayEndedFn) return;
        Entity& parent = EntityHelper::getEnforcedEntityForID(parent_id);
        onDayEndedFn(parent);
    }
    void call_night_ended() {
        if (!onNightEndedFn) return;
        Entity& parent = EntityHelper::getEnforcedEntityForID(parent_id);
        onNightEndedFn(parent);
    }

    auto& set_parent(EntityID id) {
        parent_id = id;
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
