#include "graphics_types.h"

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

bool operator<(const Vector2& l, const Vector2& r) {
	return (l.x < r.x) || ((l.x == r.x) && (l.y < r.y));
}

std::ostream& operator<<(std::ostream& os, const Vector2& v) {
	os << "(" << v.x << "," << v.y << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Vector3& v) {
	os << "(" << v.x << "," << v.y << "," << v.z << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Vector4& v) {
	os << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Color& c) {
	os << "(" << static_cast<unsigned int>(c.r) << ","
	   << static_cast<unsigned int>(c.g) << ","
	   << static_cast<unsigned int>(c.b) << ","
	   << static_cast<unsigned int>(c.a) << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Rectangle& r) {
	os << "Rectangle(" << r.x << "," << r.y << "," << r.width << "," << r.height
	   << ")";
	return os;
}

}  // namespace raylib

using raylib::Color;

namespace ext {

void draw_fps(int x, int y) { raylib::DrawFPS(x, y); }
void clear_background(raylib::Color col) { raylib::ClearBackground(col); }

void init_audio_device() { raylib::InitAudioDevice(); }
void close_audio_device() { raylib::CloseAudioDevice(); }

void set_clipboard_text(const char* text) { return raylib::SetClipboardText(text); }

const char* get_clipboard_text() { return raylib::GetClipboardText(); }

vec2 get_mouse_position() { return raylib::GetMousePosition(); }

bool is_key_pressed(int keycode) { return raylib::IsKeyPressed(keycode); }

bool is_key_down(int keycode) { return raylib::IsKeyDown(keycode); }

float get_gamepad_axis_movement(int gamepad, raylib::GamepadAxis axis) {
	return raylib::GetGamepadAxisMovement(gamepad, axis);
}

float get_mouse_wheel_move() { return raylib::GetMouseWheelMove(); }

void set_gamepad_mappings(const char* mappings) { raylib::SetGamepadMappings(mappings); }

}  // namespace ext

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