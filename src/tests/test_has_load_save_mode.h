#pragma once

#include "../components/has_load_save_mode.h"
#include "../engine/assert.h"

namespace tests {

inline void test_has_load_save_mode() {
    using Mode = HasLoadSaveMode::Mode;

    HasLoadSaveMode mode;
    VALIDATE(mode.mode() == Mode::Load, "load/save mode should start as load");
    VALIDATE(!mode.is_delete_mode(), "load mode should not be delete mode");

    VALIDATE(mode.toggle(), "toggling from load should enter delete mode");
    VALIDATE(mode.is_delete_mode(), "toggle should enable delete mode");

    VALIDATE(!mode.toggle(), "toggling from delete should leave delete mode");
    VALIDATE(mode.mode() == Mode::Load, "second toggle should restore load");
}

}  // namespace tests
