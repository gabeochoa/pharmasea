// A tiny, typed replacement for the old `GLOBALS` registry.
//
// This keeps the number of "globals" explicit, typed, and easy to grep.
// Prefer passing dependencies explicitly, but this is a pragmatic bridge.
//
// Threading note: these getters/setters only make access to the *pointer/value*
// thread-safe where protected/atomic. They do NOT make the pointed-to objects
// safe to use across threads.

#pragma once

#include <atomic>
#include <mutex>

struct Entity;
struct GameCam;
struct Map;

namespace network {
struct Server;
}

namespace globals {

// Pointers to runtime-owned objects (not thread-safe to dereference cross-thread).
void set_map(Map* map);
Map* map();

void set_game_cam(GameCam* cam);
GameCam* game_cam();

void set_camera_target(Entity* e);
Entity* camera_target();

void set_active_camera_target(Entity* e);
Entity* active_camera_target();

void set_server(network::Server* s);
network::Server* server();

// Debug/settings flags (read often, written rarely).
void set_debug_ui_enabled(bool enabled);
bool debug_ui_enabled();

void set_no_clip_enabled(bool enabled);
bool no_clip_enabled();

void set_skip_ingredient_match(bool enabled);
bool skip_ingredient_match();

// Network debug flag: user requested a mutex-protected getter/setter.
void set_network_ui_enabled(bool enabled);
bool network_ui_enabled();

}  // namespace globals

