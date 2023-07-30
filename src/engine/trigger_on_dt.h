
#pragma once

struct TriggerOnDt {
    float reset;
    float current;

    explicit TriggerOnDt(float timeToPass) : reset(timeToPass), current(0) {}

    [[nodiscard]] operator float() { return reset; }

    [[nodiscard]] bool test(float dt) {
        current -= dt;
        if (current > 0) return false;
        current = reset;
        return true;
    }
};
