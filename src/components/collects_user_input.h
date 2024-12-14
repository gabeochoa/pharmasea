
#pragma once

#include "../engine/constexpr_containers.h"
#include "../engine/keymap.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    virtual ~CollectsUserInput() {}

    auto& reset() {
        array_reset(pressed);
        return *this;
    }

    auto& write(InputName input, float value) {
        int index = magic_enum::enum_integer<InputName>(input);
        pressed[index] = value;
        return *this;
    }

    auto& publish(float dt, float camAngle) {
        if (array_contains_any_value(pressed)) {
            inputs.push_back({pressed, dt, camAngle});
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

    [[nodiscard]] InputSet read() const { return pressed; }

   private:
    // TODO I wonder if there is a way to combine all the inputs for the current
    // frame into one InputSet but we need the dts
    UserInputs inputs;

    InputSet pressed;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this));
    }
};

CEREAL_REGISTER_TYPE(CollectsUserInput);
