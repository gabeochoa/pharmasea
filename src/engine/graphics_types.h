#pragma once

#include <iosfwd>

namespace raylib {
struct Vector2;
struct Vector3;
struct Vector4;
struct Color;
struct Rectangle;
struct BoundingBox;
struct RenderTexture2D;

enum GamepadAxis : int;
enum GamepadButton : int;
enum KeyboardKey : int;

bool operator<(const Vector2& l, const Vector2& r);

std::ostream& operator<<(std::ostream& os, const Vector2& v);
std::ostream& operator<<(std::ostream& os, const Vector3& v);
std::ostream& operator<<(std::ostream& os, const Vector4& v);
std::ostream& operator<<(std::ostream& os, const Color& c);
std::ostream& operator<<(std::ostream& os, const Rectangle& r);
}  // namespace raylib

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;

using raylib::BoundingBox;
using raylib::Color;
using raylib::Rectangle;

namespace ext {

void draw_fps(int x, int y);
void clear_background(raylib::Color col);

void init_audio_device();
void close_audio_device();

void set_clipboard_text(const char* text);
[[nodiscard]] const char* get_clipboard_text();

[[nodiscard]] vec2 get_mouse_position();
[[nodiscard]] bool is_key_pressed(int keycode);
[[nodiscard]] bool is_key_down(int keycode);
[[nodiscard]] float get_gamepad_axis_movement(int gamepad, raylib::GamepadAxis axis);
[[nodiscard]] float get_mouse_wheel_move();

void set_gamepad_mappings(const char* mappings);

}  // namespace ext