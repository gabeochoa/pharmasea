
#pragma once

#include "base_component.h"

struct Entity;

typedef std::function<Entity*(vec2)> SpawnFn;

struct IsSpawner : public BaseComponent {
    virtual ~IsSpawner() {}

    auto& set_fn(SpawnFn fn) {
        spawn_fn = fn;
        return *this;
    }

    auto& set_total(int mx) {
        max_spawned = mx;
        return *this;
    }

    auto& set_time_between(float s) {
        spread = s;
        return *this;
    }

   private:
    int max_spawned = 0;
    float spread = 0;
    SpawnFn spawn_fn;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // We likely dont need to serialize anything because it should be all
        // server side info
    }
};
