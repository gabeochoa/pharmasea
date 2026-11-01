#pragma once

//
#include "base_component.h"

struct HasPatience : public BaseComponent {
    HasPatience() {
        should_subtract = false;
        update_max(20.f);
    }

    [[nodiscard]] float pct() const { return amount_left_s / max_patience_s; }

    void reset() { amount_left_s = max_patience_s; }
    void update_max(float mx) {
        max_patience_s = mx;
        amount_left_s = max_patience_s;
    }
    void pass_time(float dt) { amount_left_s -= dt; }

    void enable() { should_subtract = true; }
    void disable() { should_subtract = false; }

    [[nodiscard]] bool should_pass_time() const { return should_subtract; }

   private:
    bool should_subtract;
    float amount_left_s;
    float max_patience_s;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // TODO since we only need the pct to render,
        // we should save the pct and just serialize that
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.amount_left_s,                 //
            self.max_patience_s                 //
        );
    }
};
