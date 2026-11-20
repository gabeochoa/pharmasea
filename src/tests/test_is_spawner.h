#pragma once

#include "../components/is_spawner.h"
#include "../engine/assert.h"

namespace tests {

inline void test_is_spawner() {
    IsSpawner spawner;
    VALIDATE(spawner.state() == IsSpawner::State::Exhausted,
             "default spawner with max 0 should be exhausted");

    spawner.set_total(2);
    VALIDATE(spawner.state() == IsSpawner::State::Ready,
             "spawner below max with no countdown should be ready");

    spawner.set_time_between(5.f);
    spawner.set_fn([](Entity&, const SpawnInfo&) {});
    Entity entity;
    spawner.spawn(entity, SpawnInfo{});
    spawner.post_spawn_reset();
    VALIDATE(spawner.state() == IsSpawner::State::Cooling,
             "spawner should cool down after a spawn");

    spawner.spawn(entity, SpawnInfo{});
    VALIDATE(spawner.state() == IsSpawner::State::Exhausted,
             "spawner at max should be exhausted");
}

}  // namespace tests
