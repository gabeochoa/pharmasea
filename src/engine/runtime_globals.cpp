#include "runtime_globals.h"

namespace globals {

static std::atomic<Map*> g_world_map{nullptr};
static std::atomic<GameCam*> g_game_cam{nullptr};
static std::atomic<Entity*> g_camera_target{nullptr};
static std::atomic<Entity*> g_active_camera_target{nullptr};
static std::atomic<network::Server*> g_server{nullptr};

static std::atomic<bool> g_debug_ui_enabled{false};
static std::atomic<bool> g_no_clip_enabled{false};
static std::atomic<bool> g_skip_ingredient_match{false};

static std::mutex g_network_ui_mutex;
static bool g_network_ui_enabled = false;

void set_world_map(Map* map) {
    g_world_map.store(map, std::memory_order_release);
}
Map* world_map() { return g_world_map.load(std::memory_order_acquire); }

void set_game_cam(GameCam* cam) {
    g_game_cam.store(cam, std::memory_order_release);
}
GameCam* game_cam() { return g_game_cam.load(std::memory_order_acquire); }

void set_camera_target(Entity* e) {
    g_camera_target.store(e, std::memory_order_release);
}
Entity* camera_target() {
    return g_camera_target.load(std::memory_order_acquire);
}

void set_active_camera_target(Entity* e) {
    g_active_camera_target.store(e, std::memory_order_release);
}
Entity* active_camera_target() {
    return g_active_camera_target.load(std::memory_order_acquire);
}

void set_server(network::Server* s) {
    g_server.store(s, std::memory_order_release);
}
network::Server* server() { return g_server.load(std::memory_order_acquire); }

void set_debug_ui_enabled(bool enabled) {
    g_debug_ui_enabled.store(enabled, std::memory_order_release);
}
bool debug_ui_enabled() {
    return g_debug_ui_enabled.load(std::memory_order_acquire);
}

void set_no_clip_enabled(bool enabled) {
    g_no_clip_enabled.store(enabled, std::memory_order_release);
}
bool no_clip_enabled() {
    return g_no_clip_enabled.load(std::memory_order_acquire);
}

void set_skip_ingredient_match(bool enabled) {
    g_skip_ingredient_match.store(enabled, std::memory_order_release);
}
bool skip_ingredient_match() {
    return g_skip_ingredient_match.load(std::memory_order_acquire);
}

void set_network_ui_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(g_network_ui_mutex);
    g_network_ui_enabled = enabled;
}
bool network_ui_enabled() {
    std::lock_guard<std::mutex> lock(g_network_ui_mutex);
    return g_network_ui_enabled;
}

}  // namespace globals
