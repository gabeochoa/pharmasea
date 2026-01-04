

#pragma once

#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_ref.h"
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
        OptEntity parent_opt = parent.resolve();
        if (!parent_opt) return;
        Entity& parent_e = parent_opt.asE();
        onDayStartedFn(parent_e);
    }
    void call_night_started() {
        if (!onNightStartedFn) return;
        OptEntity parent_opt = parent.resolve();
        if (!parent_opt) return;
        onNightStartedFn(parent_opt.asE());
    }

    void call_day_ended() {
        if (!onDayEndedFn) return;
        OptEntity parent_opt = parent.resolve();
        if (!parent_opt) return;
        onDayEndedFn(parent_opt.asE());
    }
    void call_night_ended() {
        if (!onNightEndedFn) return;
        OptEntity parent_opt = parent.resolve();
        if (!parent_opt) return;
        onNightEndedFn(parent_opt.asE());
    }

    auto& set_parent(EntityID id) {
        parent.set_id(id);
        return *this;
    }

   private:
    OnDayStartedFn onDayStartedFn = nullptr;
    OnDayEndedFn onDayEndedFn = nullptr;

    OnNightStartedFn onNightStartedFn = nullptr;
    OnNightEndedFn onNightEndedFn = nullptr;

    EntityRef parent{};

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.parent                     //
        );
    }
};
