#pragma once

#include <vector>

#include "base_component.h"

struct PlayerInputQueue : public BaseComponent {
    struct MoveStep {
        float frame_dt = 0.f;
        float cam_angle = 0.f;
        float forward = 0.f;
        float back = 0.f;
        float left = 0.f;
        float right = 0.f;
    };

    struct WorkStep {
        float frame_dt = 0.f;
        float amount = 0.f;
    };

    std::vector<MoveStep> move_steps;
    std::vector<WorkStep> work_steps;

    int pickup_presses = 0;
    int rotate_presses = 0;
    int handtruck_presses = 0;

    void reset() {
        move_steps.clear();
        work_steps.clear();
        pickup_presses = 0;
        rotate_presses = 0;
        handtruck_presses = 0;
    }

    [[nodiscard]] bool has_moves() const { return !move_steps.empty(); }
    [[nodiscard]] bool has_work() const { return !work_steps.empty(); }
    [[nodiscard]] bool has_pickup() const { return pickup_presses > 0; }
    [[nodiscard]] bool has_rotate() const { return rotate_presses > 0; }
    [[nodiscard]] bool has_handtruck() const { return handtruck_presses > 0; }

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
