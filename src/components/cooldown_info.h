#pragma once

#include "../engine/assert.h"

namespace zpp::bits {
struct access;
}

// Shared timer semantics used by multiple components (AI, interactions, etc).
// TODO: Review this public API. CooldownInfo currently mixes "cooldown gating"
// semantics with "duration/progress timer" semantics; we may want clearer naming
// (e.g., TimerInfo with start/tick/reset) or to split responsibilities again if
// it grows.
struct CooldownInfo {
    bool initialized = false;
    float remaining = 0.f;  // seconds until ready
    float reset_to = 0.f;   // seconds to reset to when triggered
    bool enabled = true;

    // "Duration timer" style API (replaces AITakesTime).
    void set_time(float t) {
        VALIDATE(t > 0.f, "time must be positive");
        reset_to = t;
        remaining = t;
        initialized = true;
        enabled = true;
    }

    void tick(float dt) {
        if (!enabled) return;
        if (remaining > 0.f) remaining -= dt;
    }
    [[nodiscard]] bool ready() const { return !enabled || remaining <= 0.f; }

    // Returns true when the timer completes.
    [[nodiscard]] bool pass_time(float dt) {
        VALIDATE(initialized, "CooldownInfo was never initialized");
        tick(dt);
        return remaining <= 0.f;
    }

    void reset() { remaining = reset_to; }
    void clear() { remaining = 0.f; }
    [[nodiscard]] float pct_remaining() const {
        if (!initialized) return 1.f;
        if (reset_to <= 0.f) return 1.f;
        return remaining / reset_to;
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(  //
            self.initialized, self.remaining, self.reset_to, self.enabled);
    }
};

