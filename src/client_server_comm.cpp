
#include "client_server_comm.h"

#include "components/has_dynamic_model_name.h"
#include "components/has_fishing_game.h"
#include "components/has_subtype.h"
#include "components/is_drink.h"
#include "dataclass/ingredient.h"
#include "engine/random_engine.h"
#include "engine/runtime_globals.h"
#include "entity_helper.h"
#include "entity_type.h"
#include "libraries/recipe_library.h"
#include "network/server.h"
#include "save_file_load_fixers.h"
#include "save_game/save_game.h"

namespace server_only {

void play_sound(const vec2& location, strings::sounds::SoundId sound_id) {
    network::Server::play_sound(location, sound_id);
}

void set_show_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = globals::server();
    if (!server) return;
    server->get_map_SERVER_ONLY()->showMinimap = true;
}
void set_hide_minimap() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = globals::server();
    if (!server) return;
    server->get_map_SERVER_ONLY()->showMinimap = false;
}

void update_seed(const std::string& seed) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = globals::server();
    if (!server) return;
    server->get_map_SERVER_ONLY()->update_seed(seed);
}
std::string get_current_seed() {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client "
            "context, this is probably gonna crash");
    }
    network::Server* server = globals::server();
    if (!server) return "";
    return server->get_map_SERVER_ONLY()->seed;
}

bool save_game_to_slot(int slot) {
    if (!is_server()) {
        log_warn("save_game_to_slot called from non-server context");
        return false;
    }
    network::Server* server = globals::server();
    if (!server) return false;

    // Capture snapshot from authoritative state.
    EntityHelper::cleanup();
    Map snapshot = *(server->get_map_SERVER_ONLY());
    snapshot.was_generated = true;

    bool ok = save_game::SaveGameManager::save_slot(slot, snapshot);
    if (ok) {
        log_info("Saved game to slot {}", slot);
    } else {
        log_warn("Failed to save game to slot {}", slot);
    }
    return ok;
}

bool delete_game_slot(int slot) {
    if (!is_server()) return false;
    bool ok = save_game::SaveGameManager::delete_slot(slot);
    if (!ok) log_warn("Failed to delete save slot {}", slot);
    return ok;
}

bool load_game_from_slot(int slot) {
    if (!is_server()) {
        log_warn("load_game_from_slot called from non-server context");
        return false;
    }
    network::Server* server = globals::server();
    if (!server) return false;

    save_game::SaveGameFile loaded;
    if (!save_game::SaveGameManager::load_slot(slot, loaded)) {
        log_warn("Failed to load save slot {}", slot);
        return false;
    }

    // Install entities into the authoritative entity list and mark generated
    // so the generator does not wipe the loaded snapshot.
    Map& server_map = *(server->get_map_SERVER_ONLY());
    server_map.showMinimap = loaded.map_snapshot.showMinimap;

    server_map.seed = loaded.map_snapshot.seed;
    server_map.hashed_seed = loaded.map_snapshot.hashed_seed;
    server_map.last_generated = loaded.map_snapshot.last_generated;
    server_map.was_generated = true;
    RandomEngine::set_seed(server_map.seed);

    // Fix up containers that loaded with EntityType::Unknown
    server_only::run_all_post_load_helpers();

    EntityHelper::invalidateCaches();

    // Push an update quickly so clients snap to the new world.
    server->force_send_map_state();

    log_info("Loaded save slot {} (seed='{}')", slot, server_map.seed);
    return true;
}
}  // namespace server_only
