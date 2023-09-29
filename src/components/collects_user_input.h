
#pragma once

#include "../engine/keymap.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    virtual ~CollectsUserInput() {}

    auto& reset() {
        pressed.reset();
        return *this;
    }

    auto& write(InputName input) {
        int index = magic_enum::enum_integer<InputName>(input);
        pressed[index] = true;
        return *this;
    }

    auto& publish(float dt, float camAngle) {
        if (pressed.any()) {
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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
