
#pragma once

#include "../external_include.h"
//
#include "event.h"
#include "raylib.h"
#include "singleton.h"

enum MouseInputType {
    ButtonPressed,
    ButtonReleased,
    ButtonDown,
    ButtonUp,
    MousePosition,
    MouseWheelMovement,
};

SINGLETON_FWD(MouseMap)
struct MouseMap {
    SINGLETON(MouseMap)

    static void forEachMouseButtonPressed(
        const std::function<void(Event&)>& cb) {
        size_t num_inputs = magic_enum::enum_count<Mouse::MouseCode>();
        for (int i = 0; i < (int) num_inputs; i++) {
            bool pressed = raylib::IsMouseButtonPressed(i);
            if (pressed) {
                Mouse::MouseButtonPressedEvent* event =
                    new Mouse::MouseButtonPressedEvent(i);
                cb(*event);
                delete event;
            }
        }
    }
    static void forEachMouseButtonReleased(
        const std::function<void(Event&)>& cb) {
        size_t num_inputs = magic_enum::enum_count<Mouse::MouseCode>();
        for (int i = 0; i < (int) num_inputs; i++) {
            bool released = raylib::IsMouseButtonReleased(i);
            if (released) {
                Mouse::MouseButtonReleasedEvent* event =
                    new Mouse::MouseButtonReleasedEvent(i);
                cb(*event);
                delete event;
            }
        }
    }
    static void forEachMouseButtonDown(const std::function<void(Event&)>& cb) {
        size_t num_inputs = magic_enum::enum_count<Mouse::MouseCode>();
        for (int i = 0; i < (int) num_inputs; i++) {
            bool down = raylib::IsMouseButtonDown(i);
            if (down) {
                Mouse::MouseButtonDownEvent* event =
                    new Mouse::MouseButtonDownEvent(i);
                cb(*event);
                delete event;
            }
        }
    }
    static void forEachMouseButtonUp(const std::function<void(Event&)>& cb) {
        size_t num_inputs = magic_enum::enum_count<Mouse::MouseCode>();
        for (int i = 0; i < (int) num_inputs; i++) {
            bool up = raylib::IsMouseButtonUp(i);
            if (up) {
                Mouse::MouseButtonUpEvent* event =
                    new Mouse::MouseButtonUpEvent(i);
                cb(*event);
                delete event;
            }
        }
    }

    static void forEachMouseInput(const std::function<void(Event&)>& cb) {
        size_t num_inputs = magic_enum::enum_count<MouseInputType>();
        for (size_t i = 0; i < num_inputs; i++) {
            auto mit = magic_enum::enum_value<MouseInputType>(i);
            switch (mit) {
                case ButtonPressed:
                    forEachMouseButtonPressed(cb);
                    break;
                case ButtonReleased:
                    forEachMouseButtonReleased(cb);
                    break;
                case ButtonDown:
                    forEachMouseButtonDown(cb);
                    break;
                case ButtonUp:
                    forEachMouseButtonUp(cb);
                    break;
                case MousePosition: {
                    vec2 dm = raylib::GetMouseDelta();
                    if (dm.x > 0 && dm.y > 0) {
                        vec2 mp = raylib::GetMousePosition();
                        Mouse::MouseMovedEvent* event =
                            new Mouse::MouseMovedEvent(mp.x, mp.y);
                        cb(*event);
                        delete event;
                    }
                } break;
                case MouseWheelMovement: {
                    vec2 mwm = raylib::GetMouseWheelMoveV();
                    if (mwm.x > 0 && mwm.y > 0) {
                        Mouse::MouseScrolledEvent* event =
                            new Mouse::MouseScrolledEvent(mwm.x, mwm.y);
                        cb(*event);
                        delete event;
                    }
                } break;
            }
        }
    }
};
