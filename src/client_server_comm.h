#pragma once

#include <string>

#include "external_include.h"
#include "strings.h"

namespace server_only {
void play_sound(const vec2& location, strings::sounds::SoundId sound_id);
void set_show_minimap();
void set_hide_minimap();
void update_seed(const std::string& seed);
std::string get_current_seed();

// Save/Load (host/server thread only).
bool save_game_to_slot(int slot);
bool load_game_from_slot(int slot);
bool delete_game_slot(int slot);
}  // namespace server_only
