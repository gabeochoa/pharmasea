
#pragma once

struct TriggerOnDt {
    float reset;
    float current;

    TriggerOnDt(float timeToPass) : reset(timeToPass) {}

    [[nodiscard]] bool test(float dt) {
        current -= dt;
        if (current > 0) return false;
        current = reset;
        return true;
    }
};
