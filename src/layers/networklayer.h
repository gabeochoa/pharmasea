
#pragma once

#include <regex>

#include "../engine.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../engine/toastmanager.h"
#include "../engine/ui/ui.h"
#include "../network/network.h"

using namespace ui;

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    LayoutBox role_selector;
    LayoutBox username_picker;
    LayoutBox connected_screen;
    LayoutBox ip_input_screen;

    std::shared_ptr<network::Info> network_info;

    NetworkLayer()
        : Layer("Network"),
          ui_context(std::make_shared<ui::UIContext>()),
          role_selector(load_ui("resources/html/role_selector.html", WIN_R())),
          username_picker(
              load_ui("resources/html/username_picker.html", WIN_R())),
          connected_screen(
              load_ui("resources/html/connected_screen.html", WIN_R())),
          ip_input_screen(
              load_ui("resources/html/ip_input_screen.html", WIN_R())),
          network_info(std::make_shared<network::Info>()) {}

    virtual ~NetworkLayer() {}

    bool onCharPressedEvent(CharPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context.get()->process_char_press_event(event);
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float dt) override {
        // NOTE: this has to go above the checks since it always has to run
        network_info->tick(dt);

        if (MenuState::get().is_not(menu::State::Network)) return;
        // if we get here, then user clicked "join"
    }

    void handle_announcements() {
        if (!network_info->client) return;

        for (auto info : network_info->client->announcements) {
            log_info("Announcement: {}", info.message);
            // TODO add support for other annoucement types
            TOASTS.push_back({.msg = info.message, .timeToShow = 10});
        }
        network_info->client->announcements.clear();
    }

    void process_on_click(const std::string& id) {
        log_info("on click {}", id);
        switch (hashString(id)) {
            case hashString(strings::i18n::BACK_BUTTON):
                MenuState::get().clear_history();
                MenuState::get().set(menu::State::Root);
                break;
            case hashString(strings::i18n::HOST):
                network_info->set_role(network::Info::Role::s_Host);
                break;
            case hashString(strings::i18n::JOIN):
                network_info->set_role(network::Info::Role::s_Client);
                break;
            case hashString(strings::i18n::EDIT):
                network_info->unlock_username();
                break;
            case hashString(strings::i18n::LOCK_IN):
                network_info->lock_in_username();
                break;
            case hashString(strings::i18n::SHOW_IP):
                network_info->show_ip_addr = !network_info->show_ip_addr;
                break;
            case hashString(strings::i18n::COPY_IP):
                ext::set_clipboard_text(network_info->my_ip_address.c_str());
                break;
            case hashString(strings::i18n::START):
                MenuState::get().set(menu::State::Game);
                GameState::get().set(game::State::Lobby);
                break;
            case hashString(strings::i18n::DISCONNECT):
                network_info.reset(new network::Info());
                break;
        }
    }

    elements::InputDataSource dataFetcher(const std::string& id) {
        switch (hashString(id)) {
            case hashString("username"): {
                return elements::TextfieldData{
                    Settings::get().data.username,
                };
            } break;
            case hashString("myipaddr"): {
                auto ip_addr = network_info->show_ip_addr
                                   ? network_info->my_ip_address
                                   : "***.***.***.***";
                return elements::TextfieldData{
                    ip_addr,
                };
            } break;
            case hashString("ipaddr"): {
                // TODO add support for show hide on host ip
                return elements::TextfieldData{network_info->host_ip_address()};
            } break;
            case hashString("show_hide_text"): {
                return elements::TextfieldData{
                    network_info->show_ip_addr
                        ? text_lookup(strings::i18n::HIDE_IP)
                        : text_lookup(strings::i18n::SHOW_IP),
                };
            } break;
        }
        log_warn("trying to fetch data for {} but didnt find anything", id);
        return "";
    }

    void inputProcessor(const std::string& id, elements::ElementResult result) {
        switch (hashString(id)) {
            case hashString("ShowHideIP"): {
                network_info->show_ip_addr = !network_info->show_ip_addr;
            } break;
            case hashString("ipaddr"): {
                network_info->host_ip_address() = result.as<std::string>();
            } break;
        }
    }

    LayoutBox& get_current_screen() {
        if (network_info->missing_username()) {
            return username_picker;
        }

        if (network_info->missing_role()) {
            return role_selector;
        }

        if (network_info->has_set_ip()) {
            return connected_screen;
        }
        return ip_input_screen;
    }

    virtual void onDraw(float dt) override {
        // TODO add an overlay that shows who's currently available
        // draw_network_overlay();

        if (MenuState::get().is_not(menu::State::Network)) return;
        ClearBackground(ui_context->active_theme().background);

        elements::begin(ui_context, dt);

        render_ui(
            get_current_screen(), WIN_R(),
            std::bind(&NetworkLayer::process_on_click, *this,
                      std::placeholders::_1),
            std::bind(&NetworkLayer::dataFetcher, *this, std::placeholders::_1),
            std::bind(&NetworkLayer::inputProcessor, *this,
                      std::placeholders::_1, std::placeholders::_2));

        elements::end();
    }
};
