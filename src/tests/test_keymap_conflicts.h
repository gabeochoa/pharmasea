#pragma once

#include "../engine/assert.h"
#include "../engine/keymap.h"

namespace tests {

inline void test_keymap_conflicts() {
    LayerMapping layer;
    layer[InputName::PlayerForward] = {raylib::KEY_W};
    layer[InputName::PlayerBack] = {raylib::KEY_S};

    auto conflicts = KeyMap::conflicts_in(layer, InputName::PlayerBack,
                                          AnyInput{raylib::KEY_W});
    VALIDATE(conflicts.size() == 1 &&
                 conflicts[0] == InputName::PlayerForward,
             "binding W for PlayerBack should conflict with PlayerForward");

    conflicts = KeyMap::conflicts_in(layer, InputName::PlayerForward,
                                     AnyInput{raylib::KEY_W});
    VALIDATE(conflicts.empty(), "rebinding an action to its own key is fine");

    conflicts = KeyMap::conflicts_in(layer, InputName::PlayerBack,
                                     AnyInput{raylib::KEY_J});
    VALIDATE(conflicts.empty(), "an unused key should not conflict");
}

}  // namespace tests
