
#pragma once

#include "../engine/constexpr_containers.h"
#include "../engine/keymap.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    auto& reset_current_frame() {
        array_reset(current_frame.presses);
        current_frame.frame_dt = 0.f;
        current_frame.cam_angle = 0.f;
        return *this;
    }

    auto& reset() { return reset_current_frame(); }

    auto& write(InputName input, float value) {
        int index = magic_enum::enum_integer<InputName>(input);
        current_frame.presses[index] = value;
        return *this;
    }

    auto& set_frame_metadata(float dt, float camAngle) {
        current_frame.frame_dt = dt;
        current_frame.cam_angle = camAngle;
        return *this;
    }

    [[nodiscard]] bool has_current_frame_input() const {
        return array_contains_any_value(current_frame.presses);
    }

    auto& queue_current_frame() {
        if (array_contains_any_value(current_frame.presses)) {
            inputs.push_back(current_frame);
        }
        return *this;
    }

    auto& publish(float dt, float camAngle) {
        set_frame_metadata(dt, camAngle);
        queue_current_frame();
        reset_current_frame();
        return *this;
    }

    [[nodiscard]] bool empty() const {
        bool empty = inputs.empty();
        return empty;
    }

    [[nodiscard]] UserInputs& inputs_NETWORK_ONLY() { return inputs; }
    void clear() { inputs.clear(); }
    [[nodiscard]] size_t pending_count() const { return inputs.size(); }

    bool consume_next(UserInputSnapshot& out) {
        if (inputs.empty()) return false;
        out = inputs.front();
        inputs.erase(inputs.begin());
        return true;
    }

    [[nodiscard]] UserInputSnapshot read() const { return current_frame; }

    void set_inputs_SERVER_ONLY(const UserInputs& new_inputs) {
        this->inputs = new_inputs;
    }

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
