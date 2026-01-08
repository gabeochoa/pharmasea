

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
#include <raymath.h>
#include <rlgl.h>

#include <ostream>

// NOTE: why doesnt RaylibOpOverloads do this?
inline bool operator<(const Vector2& l, const Vector2& r) {
    return (l.x < r.x) || ((l.x == r.x) && (l.y < r.y));
}

// Stream operators to enable logging/formatting via fmt/ostream
inline std::ostream& operator<<(std::ostream& os, const Vector2& v) {
    os << "(" << v.x << "," << v.y << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Vector3& v) {
    os << "(" << v.x << "," << v.y << "," << v.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Vector4& v) {
    os << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Color& c) {
    os << "(" << static_cast<unsigned int>(c.r) << ","
       << static_cast<unsigned int>(c.g) << ","
       << static_cast<unsigned int>(c.b) << ","
       << static_cast<unsigned int>(c.a) << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Rectangle& r) {
    os << "Rectangle(" << r.x << "," << r.y << "," << r.width << "," << r.height
       << ")";
    return os;
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
using raylib::Rectangle;
// Use afterhours::Color instead of raylib::Color directly
#ifndef AFTER_HOURS_USE_RAYLIB
#define AFTER_HOURS_USE_RAYLIB
#endif
#include "afterhours/src/plugins/color.h"
using Color = afterhours::Color;

// Forward declaration for synthetic key state (bypass functionality)
namespace input_injector {
bool is_key_synthetically_down(int keycode);
bool consume_synthetic_press(int keycode);
}  // namespace input_injector

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
    bool synthetic = input_injector::consume_synthetic_press(keycode);
    bool real = raylib::IsKeyPressed(keycode);
    return synthetic || real;
}

[[nodiscard]] inline bool is_key_down(int keycode) {
    // Check synthetic key state first (for bypass)
    if (input_injector::is_key_synthetically_down(keycode)) {
        return true;
    }
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
