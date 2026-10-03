#pragma once

#include "../../components/bypass_automation_state.h"
#include "../../entities/entity_helper.h"
#include "../../external_include.h"
#include "../../globals.h"
#include "../../network/network.h"
#include "../log.h"
#include "../settings.h"
#include "../statemanager.h"
#include "../svg_renderer.h"
#include "simulated_input.h"

// Helper to inject clicks for bypass functionality
// This runs independently and doesn't modify the layers it clicks on
namespace bypass_helper {

inline void inject_clicks_for_bypass(float dt) {
    bool active = BYPASS_MENU;

#if ENABLE_DEV_FLAGS
    BypassAutomationState* state = nullptr;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        state = &sophie.addComponentIfMissing<BypassAutomationState>();
        active = state->bypass_enabled && !state->completed;
        if (state->completed) {
            BYPASS_MENU = false;
        } else if (state->bypass_enabled) {
            BYPASS_MENU = true;
        }
    } catch (...) {
    }
#else
    BypassAutomationState* state = nullptr;
#endif

    if (!active) return;

    static float fb_startup_delay = 0.0f;
    static float fb_host_role_set_time = -1.0f;
    static bool fb_play_clicked = false;
    static bool fb_username_saved = false;
    static bool fb_host_clicked = false;
    static bool fb_start_clicked = false;
    static bool fb_lobby_w_held = false;
    static float fb_lobby_entry_time = -1.0f;
    static bool fb_ingame_w_held = false;
    static float fb_ingame_entry_time = -1.0f;

    float& startup_delay = state ? state->startup_delay : fb_startup_delay;
    float& host_role_set_time =
        state ? state->host_role_set_time : fb_host_role_set_time;
    bool& play_clicked = state ? state->play_clicked : fb_play_clicked;
    bool& username_saved = state ? state->username_saved : fb_username_saved;
    bool& host_clicked = state ? state->host_clicked : fb_host_clicked;
    bool& start_clicked = state ? state->start_clicked : fb_start_clicked;
    bool& lobby_w_held = state ? state->lobby_w_held : fb_lobby_w_held;
    float& lobby_entry_time =
        state ? state->lobby_entry_time : fb_lobby_entry_time;
    bool& ingame_w_held = state ? state->ingame_w_held : fb_ingame_w_held;
    float& ingame_entry_time =
        state ? state->ingame_entry_time : fb_ingame_entry_time;

    constexpr float STARTUP_DELAY = 1.0f;
    if (startup_delay < STARTUP_DELAY) {
        float prev = startup_delay;
        startup_delay += dt;
        if (prev < STARTUP_DELAY && startup_delay >= STARTUP_DELAY) {
            log_info(
                "Bypass: Startup delay complete, starting bypass sequence");
        }
        return;
    }

    if (MenuState::get().is(menu::State::Root)) {
        if (!play_clicked) {
            log_info("Bypass: Clicking Play button");
            SVGRenderer svg("main_menu");
            Rectangle play_rect = svg.rect("PlayButton");
            input_injector::schedule_mouse_click_at(play_rect);
            play_clicked = true;
        }
        return;
    }

    if (MenuState::get().is(menu::State::Game) &&
        GameState::get().is(game::State::Lobby)) {
        if (lobby_entry_time < 0.0f) {
            lobby_entry_time = 0.0f;
            log_info(
                "Bypass: Entered Lobby state (MenuState::Game, "
                "GameState::Lobby)");
        }
        lobby_entry_time += dt;

        constexpr float LOBBY_SPAWN_DELAY = 1.0f;
        if (!lobby_w_held && lobby_entry_time >= LOBBY_SPAWN_DELAY) {
            log_info(
                "Bypass: Lobby spawn delay complete, holding W for 2.0s to "
                "move to trigger area");
            input_injector::hold_key_for_duration(raylib::KEY_W, 0.50f);
            lobby_w_held = true;
        }
        return;
    }

    if (MenuState::get().is(menu::State::Game) &&
        GameState::get().is(game::State::Lobby)) {
        if (ingame_entry_time < 0.0f) {
            ingame_entry_time = 0.0f;
            log_info(
                "Bypass: Entered Lobby state for 250ms W hold "
                "(MenuState::Game, GameState::Lobby)");
        }
        ingame_entry_time += dt;

        constexpr float INGAME_SPAWN_DELAY = 1.0f;
        if (!ingame_w_held && ingame_entry_time >= INGAME_SPAWN_DELAY) {
            log_info(
                "Bypass: InGame spawn delay complete, holding W for 250ms");
            input_injector::hold_key_for_duration(raylib::KEY_W, 0.25f);
            ingame_w_held = true;
        }
        return;
    }

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

    if (!username_saved && network_info->missing_username()) {
        if (Settings::get().data.username.empty()) {
            Settings::get().data.username = "Player";
            log_info("Bypass: Set default username to 'Player'");
        }
        log_info("Bypass: Clicking Save button to save username");
        SVGRenderer username_screen("username_screen");
        Rectangle save_rect = username_screen.rect("SaveButton");
        input_injector::schedule_mouse_click_at(save_rect);
        username_saved = true;
    }

    if (!host_clicked && !network_info->missing_username() &&
        network_info->missing_role()) {
        log_info("Bypass: Clicking Host button");
        SVGRenderer network_selection_screen("network_selection_screen");
        Rectangle host_rect = network_selection_screen.rect("HostButton");
        input_injector::schedule_mouse_click_at(host_rect);
        host_clicked = true;
        host_role_set_time = 0.0f;
    }

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
            input_injector::schedule_mouse_click_at(start_rect);
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
