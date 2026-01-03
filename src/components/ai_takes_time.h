#pragma once

#include "../engine/assert.h"
#include "base_component.h"

struct AITakesTime {
    bool initialized = false;
    float totalTime = -1.f;
    float timeRemaining = -1.f;

    void set_time(float t) {
        VALIDATE(t > 0, "time must be positive");
        totalTime = t;
        timeRemaining = t;
        initialized = true;
    }

    [[nodiscard]] bool pass_time(float dt) {
        VALIDATE(initialized, "AITakesTime was never initialized");
        timeRemaining -= dt;
        return timeRemaining <= 0.f;
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(self.initialized, self.totalTime, self.timeRemaining);
    }
};

