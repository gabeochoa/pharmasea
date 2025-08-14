#pragma once

#include <string>

#include "external_include.h"

namespace server_only {
void play_sound(const vec2& location, strings::sounds::SoundId sound_id);
void set_show_minimap();
void set_hide_minimap();
void update_seed(const std::string& seed);
std::string get_current_seed();
}  // namespace server_only
