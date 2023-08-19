
#pragma once

#include <regex>

#include "../engine.h"
#include "../engine/network/webrequest.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../engine/toastmanager.h"
#include "../engine/ui.h"
#include "../network/network.h"

using namespace ui;

inline bool validate_ip(const std::string& ip) {
    // TODO this should fail for 00000.00000.0000 but shouldnt
    //      this too 000000000000
    //
    // NOTE: this is const static cause its faster :)
    const static std::regex ip_regex(
        R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, ip_regex);
}

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    std::shared_ptr<network::Info> network_info;
    std::string my_ip_address;
    bool should_show_host_ip = false;

    NetworkLayer() : Layer("Network") {
        ui_context = std::make_shared<ui::UIContext>();

        network::Info::init_connections();
        network_info = std::make_shared<network::Info>();
        if (network::ENABLE_REMOTE_IP) {
            my_ip_address = network::get_remote_ip_address().value_or("");
        } else {
            my_ip_address = "(DEV) network disabled";
        }
        if (!Settings::get().data.username.empty()) {
            network_info->lock_in_username();
        }
    }

    void setup_mp_test() {
        if (network::mp_test::run_init++ < (120 * 3)) return;

        if (network::mp_test::is_host) {
            MenuState::get().set(menu::State::Network);
            network_info->set_role(network::Info::Role::s_Host);

            MenuState::get().set(menu::State::Game);
            GameState::get().set(game::State::Lobby);
        } else {
            MenuState::get().set(menu::State::Network);
            network_info->set_role(network::Info::Role::s_Client);
        }

        network_info->lock_in_username();
        network_info->host_ip_address() = Settings::get().last_used_ip();
        network_info->lock_in_ip();
    }

    virtual ~NetworkLayer() { network::Info::shutdown_connections(); }

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
        if (network::mp_test::enabled) {
            setup_mp_test();
        }
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

    void draw_username_picker(float) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 30);
        content = rect::lpad(content, 30);

        auto [username, controls] = rect::hsplit(content, 20);

        username = rect::rpad(username, 50);
        auto [label, name] = rect::hsplit<2>(username);

        controls = rect::tpad(controls, 50);
        controls = rect::rpad(controls, 30);
        auto [lock, back] = rect::hsplit<2>(controls, 20);

        text(Widget{label}, text_lookup(strings::i18n::USERNAME));

        if (auto result = textfield(
                Widget{name},
                TextfieldData{Settings::get().data.username,
                              [](const std::string& content) {
                                  if (content.size() >=
                                      network::MAX_NAME_LENGTH)
                                      return TextfieldValidationDecisionFlag::
                                          StopNewInput;
                                  return TextfieldValidationDecisionFlag::Valid;
                              }});
            result) {
            Settings::get().data.username = result.as<std::string>();
        }

        if (button(Widget{lock}, text_lookup(strings::i18n::LOCK_IN))) {
            network_info->lock_in_username();
        }

        if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }
    }

    void draw_role_selector_screen(float) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 30);
        content = rect::bpad(content, 30);

        auto [buttons, username] = rect::vsplit(content, 40);

        // button
        {
            auto [host, join, back] =
                rect::hsplit<3>(rect::rpad(rect::lpad(buttons, 15), 20), 20);

            if (button(Widget{host}, text_lookup(strings::i18n::HOST))) {
                network_info->set_role(network::Info::Role::s_Host);
            }
            if (button(Widget{join}, text_lookup(strings::i18n::JOIN))) {
                network_info->set_role(network::Info::Role::s_Client);
            }
            if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON))) {
                MenuState::get().clear_history();
                MenuState::get().set(menu::State::Root);
            }
        }

        {
            username = rect::bpad(username, 30);
            auto [label, name, edit] =
                rect::vsplit<3>(rect::rpad(username, 40));

            text(Widget{label}, text_lookup(strings::i18n::USERNAME));

            text(Widget{name}, Settings::get().data.username);

            if (button(Widget{edit}, text_lookup(strings::i18n::EDIT))) {
                network_info->unlock_username();
            }
        }
    }

    void draw_connected_screen(float dt) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 30);
        content = rect::rpad(content, 80);
        content = rect::lpad(content, 20);
        content = rect::bpad(content, 80);

        auto [connected_players, buttons, ip_addr, username] =
            rect::hsplit<4>(content);

        // Draw connected players
        {
            auto players = rect::hsplit<4>(connected_players);

            text(Widget{players[0]}, Settings::get().data.username);

            /*
            for (auto kv : network_info->client->remote_players) {
                // TODO figure out why there are null rps
                if (!kv.second) continue;
                auto player_text = ui_context->own(
                    Widget(MK_UUID_LOOP(id, ROOT_ID, kv.first),
                           Size_Px(120.f, 0.5f), Size_Px(100.f, 1.f)));
                text(*player_text,
                     fmt::format("{}({})", kv.second->get<HasName>().name(),
                                 kv.first));
            }
            */
        }

        {
            buttons = rect::rpad(buttons, 30);

            if (network_info->is_host()) {
                auto [start, disconnect] = rect::hsplit<2>(buttons);

                if (button(Widget{start}, text_lookup(strings::i18n::START))) {
                    MenuState::get().set(menu::State::Game);
                    GameState::get().set(game::State::Lobby);
                }
                if (button(Widget{disconnect},
                           text_lookup(strings::i18n::DISCONNECT))) {
                    network_info.reset(new network::Info());
                }
            } else {
                auto disconnect = buttons;
                if (button(Widget{disconnect},
                           text_lookup(strings::i18n::DISCONNECT))) {
                    network_info.reset(new network::Info());
                }
            }
        }

        // your ip is .... show copy
        {
            ip_addr = rect::bpad(ip_addr, 50);
            auto [label, control] = rect::vsplit<2>(ip_addr);

            auto ip = should_show_host_ip ? my_ip_address : "***.***.***.***";
            text(Widget{label},
                 // TODO not translated
                 fmt::format("Your IP is: {}", ip));

            auto [check, copy] = rect::vsplit<2>(control);

            // TODO default value wont be setup correctly without this
            // bool sssb = Settings::get().data.show_streamer_safe_box;
            std::string show_hide_host_ip_text =
                should_show_host_ip ? text_lookup(strings::i18n::HIDE_IP)
                                    : text_lookup(strings::i18n::SHOW_IP);
            if (auto result =
                    checkbox(Widget{check},
                             CheckboxData{.selected = should_show_host_ip,
                                          .content = show_hide_host_ip_text});
                result) {
                should_show_host_ip = !should_show_host_ip;
            }

            if (button(Widget{copy}, text_lookup(strings::i18n::COPY_IP))) {
                ext::set_clipboard_text(my_ip_address.c_str());
            }
        }

        // TODO add button to edit as long as you arent currently
        // hosting people?
        draw_username_with_edit(username, dt);
    }

    void draw_username_with_edit(Rectangle parent, float dt) {
        using namespace ui;
        auto [label, name, edit] = rect::vsplit<3>(parent);

        text(Widget{label}, text_lookup(strings::i18n::USERNAME));

        text(Widget{name}, Settings::get().data.username);

        if (button(Widget{edit}, text_lookup(strings::i18n::EDIT))) {
            network_info->unlock_username();
        }
    }

    void draw_ip_input_screen(float dt) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 30);
        content = rect::lpad(content, 20);
        content = rect::rpad(content, 80);

        auto [username, controls] = rect::hsplit(content, 40);

        draw_username_with_edit(username, dt);

        // TODO add showhide button

        // IP addr input
        auto [ip, buttons, back] = rect::hsplit<3>(controls);
        {
            auto [label, control] = rect::vsplit<2>(ip);

            text(Widget{label}, text_lookup(strings::i18n::ENTER_IP));

            if (auto result = textfield(
                    Widget{control},
                    TextfieldData{
                        network_info->host_ip_address(),
                        [](const std::string& content) {
                            // xxx.xxx.xxx.xxx
                            if (content.size() > 15) {
                                return TextfieldValidationDecisionFlag::
                                    StopNewInput;
                            }
                            if (validate_ip(content)) {
                                return TextfieldValidationDecisionFlag::Valid;
                            }
                            return TextfieldValidationDecisionFlag::Invalid;
                        }});
                result) {
                network_info->host_ip_address() = result.as<std::string>();
            }
        }

        // Buttons
        {
            buttons = rect::rpad(buttons, 50);
            auto [load_ip, connect] = rect::hsplit<2>(buttons);

            if (button(Widget{load_ip},
                       text_lookup(strings::i18n::LOAD_LAST_IP))) {
                network_info->host_ip_address() =
                    Settings::get().last_used_ip();
            }

            if (button(Widget{connect}, text_lookup(strings::i18n::CONNECT))) {
                Settings::get().update_last_used_ip_address(
                    network_info->host_ip_address());
                network_info->lock_in_ip();
            }
        }

        // Back button
        {
            back = rect::rpad(back, 50);
            if (button(Widget{back}, text_lookup(strings::i18n::BACK_BUTTON))) {
                network_info->unlock_username();
            }
        }
    }

    void draw_screen(float dt) {
        if (network_info->missing_username()) {
            draw_username_picker(dt);
            return;
        }

        if (network_info->missing_role()) {
            draw_role_selector_screen(dt);
            return;
        }

        if (network_info->has_set_ip()) {
            draw_connected_screen(dt);
            return;
        }

        draw_ip_input_screen(dt);
    }

    virtual void onDraw(float dt) override {
        // TODO add an overlay that shows who's currently available
        // draw_network_overlay();

        if (MenuState::get().is_not(menu::State::Network)) return;

        ext::clear_background(ui_context->active_theme().background);

        using namespace ui;

        begin(ui_context, dt);
        draw_screen(dt);
        end();

        handle_announcements();
    }
};
