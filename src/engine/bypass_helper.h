#pragma once

#include "../engine/settings.h"
#include "../engine/svg_renderer.h"
#include "../globals.h"
#include "../network/network.h"
#include "input_injector.h"
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

    // Wait 1 second for game to fully initialize before injecting clicks
    constexpr float STARTUP_DELAY = 1.0f;
    if (startup_delay < STARTUP_DELAY) {
        startup_delay += dt;
        return;
    }

    // Step 0: Inject click on Play button if at root menu
    if (MenuState::get().is(menu::State::Root)) {
        if (!play_clicked) {
            SVGRenderer svg("main_menu");
            Rectangle play_rect = svg.rect("PlayButton");
            input_injector::inject_mouse_click_at(play_rect);
            play_clicked = true;
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
        return;
    }

    if (!network_info) return;

    // Step 1: Handle username if missing
    if (!username_saved && network_info->missing_username()) {
        // Set default username if none exists
        if (Settings::get().data.username.empty()) {
            Settings::get().data.username = "Player";
        }
        // Inject click on Save button
        SVGRenderer username_screen("username_screen");
        Rectangle save_rect = username_screen.rect("SaveButton");
        input_injector::inject_mouse_click_at(save_rect);
        username_saved = true;
    }

    // Step 2: Inject click on Host button
    if (!host_clicked && !network_info->missing_username() &&
        network_info->missing_role()) {
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
            SVGRenderer lobby_screen("lobby_screen");
            Rectangle start_rect = lobby_screen.rect("StartButton");
            input_injector::inject_mouse_click_at(start_rect);
            start_clicked = true;
        }
    }
}

}  // namespace bypass_helper
