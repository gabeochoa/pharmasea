

#pragma once

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace raylib {

#include <raylib.h>
#include <rlgl.h>

#undef RAYLIB_OP_OVERLOADS_RAYGUI
#include <RaylibOpOverloads.h>

// NOTE: why doesnt RaylibOpOverloads do this?
inline bool operator<(const Vector2& l, const Vector2& r) {
    return (l.x < r.x) || ((l.x == r.x) && (l.y < r.y));
}

}  // namespace raylib

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

using raylib::BoundingBox;
using raylib::Color;
using raylib::Rectangle;

namespace ext {

// Drawing

inline void draw_fps(int x, int y) { raylib::DrawFPS(x, y); }
inline void clear_background(raylib::Color col) {
    raylib::ClearBackground(col);
}

// AudioDevice

inline void init_audio_device() { raylib::InitAudioDevice(); }
inline void close_audio_device() { raylib::CloseAudioDevice(); }

// Input Related Functions

inline void set_clipboard_text(const char* text) {
    return raylib::SetClipboardText(text);
}

[[nodiscard]] inline auto get_clipboard_text() {
    return raylib::GetClipboardText();
}

[[nodiscard]] inline vec2 get_mouse_position() {
    return raylib::GetMousePosition();
}

[[nodiscard]] inline bool is_key_pressed(int keycode) {
    return raylib::IsKeyPressed(keycode);
}

[[nodiscard]] inline bool is_key_down(int keycode) {
    return raylib::IsKeyDown(keycode);
}

[[nodiscard]] inline float get_gamepad_axis_movement(int gamepad,
                                                     raylib::GamepadAxis axis) {
    return raylib::GetGamepadAxisMovement(gamepad, axis);
}

[[nodiscard]] inline float get_mouse_wheel_move() {
    return raylib::GetMouseWheelMove();
}

inline void set_gamepad_mappings(const char* mappings) {
    raylib::SetGamepadMappings(mappings);
}

}  // namespace ext
