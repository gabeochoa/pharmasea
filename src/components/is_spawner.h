
#pragma once

#include "base_component.h"

struct Entity;

typedef std::function<void(vec2)> SpawnFn;

struct IsSpawner : public BaseComponent {
    virtual ~IsSpawner() {}

    [[nodiscard]] bool hit_max() const { return num_spawned >= max_spawned; }

    auto& reset_num_spawned() {
        // TODO add max to progression_manager
        num_spawned = 0;
        return *this;
    }

    auto& set_fn(SpawnFn fn) {
        spawn_fn = fn;
        return *this;
    }

    auto& set_total(int mx) {
        max_spawned = mx;
        return *this;
    }

    // TODO probably need a thing to specify the units
    auto& set_time_between(float s) {
        spread = s;
        countdown = spread;
        return *this;
    }

    Entity* pass_time(vec2 pos, float dt) {
        if (hit_max()) return nullptr;
        if (!spawn_fn) {
            log_warn("calling pass_time without a valid spawn function");
            return nullptr;
        }

        countdown -= dt;
        if (countdown <= 0) {
            countdown = spread;
            num_spawned++;
            spawn_fn(pos);
        }
        return nullptr;
    }

   private:
    int max_spawned = 0;
    float spread = 0;

    int num_spawned = 0;
    float countdown = 0;
    SpawnFn spawn_fn;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // We likely dont need to serialize anything because it should be all
        // server side info
    }
};
