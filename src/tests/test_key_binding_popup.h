#pragma once

#include "../engine/assert.h"
#include "../layers/settingslayer.h"

namespace tests {

inline void test_key_binding_popup() {
    using CaptureState = SettingsLayer::CaptureState;

    SettingsLayer::KeyBindingPopup popup;
    VALIDATE(popup.capture_state == CaptureState::Idle,
             "new popup should start idle");
    VALIDATE(!popup.is_capturing(),
             "idle popup should not be capturing");

    popup.capture_state = CaptureState::Capturing;
    VALIDATE(popup.is_capturing(),
             "capturing popup should report capturing");
}

}  // namespace tests
