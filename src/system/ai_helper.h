#pragma once

#include <optional>

#include "../components/has_ai_cooldown.h"
#include "../components/transform.h"
#include "../entity_helper.h"

#include "ai_entity_helpers.h"

namespace system_manager::ai {

// Rate-limit AI work in states that don't need per-frame processing.
[[nodiscard]] inline bool ai_tick_with_cooldown(Entity& entity, float dt,
                                               float reset_to_seconds) {
    HasAICooldown& cd = ensure_component<HasAICooldown>(entity);
    cd.cooldown.reset_to = reset_to_seconds;
    cd.cooldown.tick(dt);
    if (!cd.cooldown.ready()) return false;
    cd.cooldown.reset();
    return true;
}

[[nodiscard]] inline std::optional<vec2> pick_random_walkable_near(
    const Entity& e, int attempts = 50) {
    const vec2 base = e.get<Transform>().as2();
    for (int i = 0; i < attempts; ++i) {
        vec2 off = RandomEngine::get().get_vec(-10.f, 10.f);
        vec2 p = base + off;
        if (EntityHelper::isWalkable(p)) return p;
    }
    return std::nullopt;
}

}  // namespace system_manager::ai

