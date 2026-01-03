#pragma once

namespace zpp::bits {
struct access;
}

// Shared timer semantics used by multiple components (AI, interactions, etc).
struct CooldownInfo {
    float remaining = 0.f;  // seconds until ready
    float reset_to = 0.f;   // seconds to reset to when triggered
    bool enabled = true;

    void tick(float dt) {
        if (!enabled) return;
        if (remaining > 0.f) remaining -= dt;
    }
    [[nodiscard]] bool ready() const { return !enabled || remaining <= 0.f; }
    void reset() { remaining = reset_to; }
    void clear() { remaining = 0.f; }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(  //
            self.remaining, self.reset_to, self.enabled);
    }
};

