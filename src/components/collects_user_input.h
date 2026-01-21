
#pragma once

#include "../engine/constexpr_containers.h"
#include "../engine/keymap.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    auto& reset() {
        array_reset(current_frame.presses);
        return *this;
    }

    auto& write(InputName input, float value) {
        int index = magic_enum::enum_integer<InputName>(input);
        current_frame.presses[index] = value;
        return *this;
    }

    auto& publish(float dt, float camAngle) {
        if (array_contains_any_value(current_frame.presses)) {
            current_frame.frame_dt = dt;
            current_frame.cam_angle = camAngle;
            inputs.push_back(current_frame);
            reset();
        }
        return *this;
    }

    [[nodiscard]] bool empty() const {
        bool empty = inputs.empty();
        return empty;
    }

    [[nodiscard]] UserInputs& inputs_NETWORK_ONLY() { return inputs; }
    void clear() { inputs.clear(); }

    [[nodiscard]] UserInputSnapshot read() const { return current_frame; }

   private:
    // TODO I wonder if there is a way to combine all the inputs for the current
    // frame into one InputPresses but we need the dts
    UserInputs inputs;

    UserInputSnapshot current_frame;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        (void) self;
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
