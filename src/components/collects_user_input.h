
#pragma once

#include "../engine/keymap.h"
#include "base_component.h"

struct CollectsUserInput : public BaseComponent {
    virtual ~CollectsUserInput() {}

    // TODO make private at some point
    UserInputs inputs;

    InputSet pressed;

    auto& reset() {
        pressed.reset();
        return *this;
    }

    auto& write(InputName input) {
        int index = magic_enum::enum_integer<InputName>(input);
        pressed[index] = true;
        return *this;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
