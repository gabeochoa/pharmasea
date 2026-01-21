#pragma once

#include "base_component.h"

struct ControlState : public BaseComponent {
    float move_forward = 0.f;
    float move_back = 0.f;
    float move_left = 0.f;
    float move_right = 0.f;
    float frame_dt = 0.f;
    float cam_angle = 0.f;

    void reset() {
        move_forward = 0.f;
        move_back = 0.f;
        move_left = 0.f;
        move_right = 0.f;
        frame_dt = 0.f;
        cam_angle = 0.f;
    }

    [[nodiscard]] bool has_movement() const {
        return move_forward > 0.f || move_back > 0.f || move_left > 0.f ||
               move_right > 0.f;
    }

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                        //
            static_cast<BaseComponent&>(self),  //
            self.move_forward,                  //
            self.move_back,                     //
            self.move_left,                     //
            self.move_right,                    //
            self.frame_dt,                      //
            self.cam_angle                      //
        );
    }
};
