#pragma once

#include "../engine/settings.h"
#include "../engine/svg_renderer.h"
#include "../external_include.h"
#include "../globals.h"
#include "../network/network.h"
#include "input_injector.h"
#include "log.h"
#include "statemanager.h"

// Helper to inject clicks for bypass functionality
// This runs independently and doesn't modify the layers it clicks on
namespace bypass_helper {

inline void inject_clicks_for_bypass(float dt) {
    if (!BYPASS_MENU) return;

    static float startup_delay = 0.0f;
    static float host_role_set_time = -1.0f;
    static bool play_clicked = false;
    static bool username_saved = false;
    static bool host_clicked = false;
    static bool start_clicked = false;
    static bool lobby_w_held = false;
    static float lobby_entry_time = -1.0f;
    static bool ingame_w_held = false;
    static float ingame_entry_time = -1.0f;

    // Wait 1 second for game to fully initialize before injecting clicks
    constexpr float STARTUP_DELAY = 1.0f;
    if (startup_delay < STARTUP_DELAY) {
        startup_delay += dt;
        if (startup_delay >= STARTUP_DELAY) {
            log_info(
                "Bypass: Startup delay complete, starting bypass sequence");
        }
        return;
    }

    // Step 0: Inject click on Play button if at root menu
    if (MenuState::get().is(menu::State::Root)) {
        if (!play_clicked) {
            log_info("Bypass: Clicking Play button");
            SVGRenderer svg("main_menu");
            Rectangle play_rect = svg.rect("PlayButton");
            input_injector::inject_mouse_click_at(play_rect);
            play_clicked = true;
        }
        return;
    }

    // Handle lobby: move player to trigger area by holding W
    if (MenuState::get().is(menu::State::Game) &&
        GameState::get().is(game::State::Lobby)) {
        // Track when we entered the lobby
        if (lobby_entry_time < 0.0f) {
            lobby_entry_time = 0.0f;
            log_info(
                "Bypass: Entered Lobby state (MenuState::Game, "
                "GameState::Lobby)");
        }
        lobby_entry_time += dt;

        // Wait for player to spawn before moving (1 second delay)
        constexpr float LOBBY_SPAWN_DELAY = 1.0f;
        if (!lobby_w_held && lobby_entry_time >= LOBBY_SPAWN_DELAY) {
            log_info(
                "Bypass: Lobby spawn delay complete, holding W for 2.0s to "
                "move to trigger area");
            // Hold W to move to the trigger area
            input_injector::hold_key_for_duration(raylib::KEY_W, 0.50f);
            lobby_w_held = true;
        }
        // Once we've held W long enough, the player should be at the trigger
        // area and the game will transition to InGame automatically
        return;
    }

    // Handle InGame: hold W for 250ms after entering (but check for Lobby
    // state)
    if (MenuState::get().is(menu::State::Game) &&
        GameState::get().is(game::State::Lobby)) {
        // Track when we entered Lobby
        if (ingame_entry_time < 0.0f) {
            ingame_entry_time = 0.0f;
            log_info(
                "Bypass: Entered Lobby state for 250ms W hold "
                "(MenuState::Game, GameState::Lobby)");
        }
        ingame_entry_time += dt;

        // Wait for player to spawn before moving (1 second delay)
        constexpr float INGAME_SPAWN_DELAY = 1.0f;
        if (!ingame_w_held && ingame_entry_time >= INGAME_SPAWN_DELAY) {
            log_info(
                "Bypass: InGame spawn delay complete, holding W for 250ms");
            input_injector::hold_key_for_duration(raylib::KEY_W, 0.25f);
            ingame_w_held = true;
        }
        return;
    }

    // Reset flags if we're not in Network state
    if (MenuState::get().is_not(menu::State::Network)) {
        username_saved = false;
        host_clicked = false;
        start_clicked = false;
        host_role_set_time = -1.0f;
        play_clicked = false;
        lobby_w_held = false;
        lobby_entry_time = -1.0f;
        ingame_w_held = false;
        ingame_entry_time = -1.0f;
        return;
    }

    if (!network_info) return;

    // Step 1: Handle username if missing
    if (!username_saved && network_info->missing_username()) {
        // Set default username if none exists
        if (Settings::get().data.username.empty()) {
            Settings::get().data.username = "Player";
            log_info("Bypass: Set default username to 'Player'");
        }
        log_info("Bypass: Clicking Save button to save username");
        // Inject click on Save button
        SVGRenderer username_screen("username_screen");
        Rectangle save_rect = username_screen.rect("SaveButton");
        input_injector::inject_mouse_click_at(save_rect);
        username_saved = true;
    }

    // Step 2: Inject click on Host button
    if (!host_clicked && !network_info->missing_username() &&
        network_info->missing_role()) {
        log_info("Bypass: Clicking Host button");
        SVGRenderer network_selection_screen("network_selection_screen");
        Rectangle host_rect = network_selection_screen.rect("HostButton");
        input_injector::inject_mouse_click_at(host_rect);
        host_clicked = true;
        host_role_set_time = 0.0f;
    }

    // Step 3: Inject click on Start button when ready
    if (!start_clicked && network_info->has_role() && network_info->is_host() &&
        network_info->has_set_ip() && network_info->client) {
        if (host_role_set_time >= 0.0f) {
            host_role_set_time += dt;
        }

        bool has_connection = network_info->client->client_p &&
                              network_info->client->client_p->is_connected();
        bool waited_enough = host_role_set_time >= 0.5f;

        if (has_connection && waited_enough) {
            log_info("Bypass: Connection ready, clicking Start button");
            SVGRenderer lobby_screen("lobby_screen");
            Rectangle start_rect = lobby_screen.rect("StartButton");
            input_injector::inject_mouse_click_at(start_rect);
            start_clicked = true;
        } else {
            if (!has_connection) {
                static float last_log = 0.0f;
                if (host_role_set_time - last_log > 0.5f) {
                    log_info("Bypass: Waiting for connection... (time: %.2f)",
                             host_role_set_time);
                    last_log = host_role_set_time;
                }
            }
        }
    }
}

}  // namespace bypass_helper
