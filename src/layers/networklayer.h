
#pragma once

#include <memory>
#include <regex>

#include "../engine.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../engine/toastmanager.h"
#include "../engine/ui/ui.h"
#include "../network/network.h"

extern std::shared_ptr<network::Info> network_info;

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

    std::string my_ip_address;
    bool should_show_host_ip = false;

    NetworkLayer()
        : Layer("Network"), ui_context(std::make_shared<ui::UIContext>()) {
        if (network_info) {
            if (!Settings::get().data.username.empty()) {
                network_info->lock_in_username();
            }
        }
    }

    virtual ~NetworkLayer() { network::Info::shutdown_connections(); }

    bool onCharPressedEvent(CharPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context->process_char_press_event(event);
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;
        return ui_context->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Network)) return false;

        if (KeyMap::get_button(menu::State::UI, InputName::MenuBack) ==
            event.button) {
            MenuState::get().go_back();
            return true;
        }

        return ui_context->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float dt) override {
        // NOTE: this has to go above the checks since it always has to run
        network_info->tick(dt);

        if (MenuState::get().is_not(menu::State::Network)) return;
        // if we get here, then user clicked "join"
    }

    void handle_announcements() {
        if (!network_info->client) return;

        for (const auto& info : network_info->client->announcements) {
            log_info("Announcement: {}", info.message);
            // TODO add support for other annoucement types
            TOASTS.push_back({.msg = info.message, .timeToShow = 5});
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

        text(Widget{label}, TranslatableString(strings::i18n::USERNAME));

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

        if (button(Widget{lock}, TranslatableString(strings::i18n::LOCK_IN))) {
            network_info->lock_in_username();
        }

        if (button(Widget{back},
                   TranslatableString(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }
    }

    void draw_username_with_edit(Rectangle username, float) {
        username = rect::lpad(username, 25);
        username = rect::rpad(username, 75);
        username = rect::tpad(username, 35);
        username = rect::bpad(username, 85);

        auto [label, uname, _empty, edit] = rect::vsplit<4>(username, 20);

        text(Widget{label}, TranslatableString(strings::i18n::USERNAME));

        text(Widget{uname}, NO_TRANSLATE(Settings::get().data.username));

        bool show_edit_button =
            network_info->client
                ? network_info->client->remote_players.size() <= 1
                : true;

        if (show_edit_button) {
            edit = rect::tpad(edit, 20);
            edit = rect::bpad(edit, 80);
            if (button(Widget{edit}, TranslatableString(strings::i18n::EDIT))) {
                network_info->unlock_username();
            }
        }
    }

    void draw_role_selector_screen(float dt) {
        // auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};

        float bg_width = WIN_WF() * 0.6f;
        float bg_height = WIN_HF() * 0.8f;
        auto background = Rectangle{(WIN_WF() - bg_width) / 2.f,   //
                                    (WIN_HF() - bg_height) / 2.f,  //
                                    bg_width, bg_height};
        div(background, color::brownish_purple);

        auto [left, right] = rect::vsplit<2>(background);
        left = rect::rpad(left, 95);
        right = rect::lpad(right, 5);

        auto [top_left, bottom_left] = rect::hsplit<2>(left);
        auto [top_right, bottom_right] = rect::hsplit<2>(right);

        div(top_left, color::green_blue);
        div(bottom_left, color::pine_green);
        div(top_right, color::pine_green);
        div(bottom_right, color::green_blue);

        {
            auto [a, b, c, back] = rect::hsplit<4>(bottom_left);
            (void) a;
            (void) b;
            (void) c;
            if (button(Widget{back},
                       TranslatableString(strings::i18n::BACK_BUTTON))) {
                MenuState::get().clear_history();
                MenuState::get().set(menu::State::Root);
            }
        }

        draw_username_with_edit(top_left, dt);

        /*
        auto content = rect::vpad(rect::hpad(window, 20), 10);
        div(content, color::brownish_purple);

        auto [left, right] = rect::vsplit<2>(content);
        left = rect::rpad(left, 90);
        right = rect::lpad(right, 10);
        div(left, color::green_blue);
        div(right, color::pine_green);

        auto [top_left, bottom_left] = rect::hsplit<2>(left);

        auto [top_right, _2] = rect::hsplit<2>(right);

        top_right = rect::hpad(top_right, 20);
        auto [host, join] = rect::hsplit<2>(top_right, 20);

        if (button(Widget{host}, TranslatableString(strings::i18n::HOST))) {
            network_info->set_role(network::Info::Role::s_Host);
        }
        if (button(Widget{join}, TranslatableString(strings::i18n::JOIN))) {
            network_info->set_role(network::Info::Role::s_Client);
        }

        auto back = rect::all_pad(bottom_left, 20);

        // Even though the ui shows up at the top
        // we dont want the tabbing to be first, so
        // we put it here
        draw_username_with_edit(top_left, dt);
        */
    }

    void draw_connected_screen(float dt) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [username, content] = rect::hsplit(window, 33);

        auto [buttons, your_ip, player_box] = rect::vsplit<3>(content);

        /// Buttons
        {
            buttons = rect::lpad(buttons, 45);
            buttons = rect::rpad(buttons, 75);

            auto [start, _a, _b, disconnect] = rect::hsplit<4>(buttons, 30);

            if (network_info->is_host()) {
                if (button(Widget{start},
                           TranslatableString(strings::i18n::START))) {
                    MenuState::get().set(menu::State::Game);
                    GameState::get().set(game::State::Lobby);
                }
                if (button(Widget{disconnect},
                           TranslatableString(strings::i18n::DISCONNECT))) {
                    network::Info::reset_connections();
                    return;
                }
            } else {
                if (button(Widget{disconnect},
                           TranslatableString(strings::i18n::DISCONNECT))) {
                    network::Info::reset_connections();
                    return;
                }
            }
        }

        {
            player_box = rect::lpad(player_box, 20);
            player_box = rect::rpad(player_box, 60);

            auto players = rect::hsplit<4>(player_box, 15);

            int i = 0;
            for (const auto& kv : network_info->client->remote_players) {
                // TODO figure out why there are null rps
                if (!kv.second) continue;

                text(
                    Widget{players[i++]},
                    NO_TRANSLATE(fmt::format(
                        "{}({})", kv.second->get<HasName>().name(), kv.first)));
            }
        }

        // your ip is .... show copy
        if (network_info->is_host()) {
            your_ip = rect::lpad(your_ip, 20);
            your_ip = rect::rpad(your_ip, 60);

            auto [_a, label, ip_addr, control] = rect::hsplit<4>(your_ip, 30);

            // TODO not translated maybe we dont need this piece
            // is the format XXX.XXX just obvious its an ip address
            text(Widget{label},
                 TODO_TRANSLATE("Your IP is:", TodoReason::Format));

            auto ip = should_show_host_ip ? network::my_remote_ip_address
                                          : "***.***.***.***";
            text(Widget{ip_addr}, NO_TRANSLATE(ip));

            auto [check, copy] = rect::vsplit<2>(control, 5);

            // TODO default value wont be setup correctly without this
            // bool sssb = Settings::get().data.show_streamer_safe_box;
            std::string show_hide_host_ip_text = should_show_host_ip
                                                     ? strings::i18n::HIDE_IP
                                                     : strings::i18n::SHOW_IP;
            if (auto result =
                    checkbox(Widget{check},
                             CheckboxData{.selected = should_show_host_ip,
                                          .content = show_hide_host_ip_text});
                result) {
                should_show_host_ip = !should_show_host_ip;
            }

            if (button(Widget{copy},
                       TranslatableString(strings::i18n::COPY_IP))) {
                ext::set_clipboard_text(my_ip_address.c_str());
            }
        }

        // Even though the ui shows up at the top
        // we dont want the tabbing to be first, so
        // we put it here
        draw_username_with_edit(username, dt);
    }

    void draw_ip_input_screen(float dt) {
        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [username, body] = rect::hsplit(window, 33);
        body = rect::lpad(body, 20);
        body = rect::rpad(body, 80);

        auto [ip, buttons, back] = rect::hsplit<3>(body, 30);

        // TODO add showhide button
        // IP addr input
        {
            ip = rect::tpad(ip, 30);
            ip = rect::bpad(ip, 30);
            ip = rect::rpad(ip, 80);

            auto [label, control] = rect::vsplit<2>(ip);

            text(Widget{label}, TranslatableString(strings::i18n::ENTER_IP));

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
            auto [bs, _a, _b] = rect::vsplit<3>(buttons);
            buttons = bs;

            auto [load_ip, connect] = rect::hsplit<2>(buttons);

            if (button(Widget{load_ip},
                       TranslatableString(strings::i18n::LOAD_LAST_IP))) {
                network_info->host_ip_address() =
                    Settings::get().last_used_ip();
            }

            if (button(Widget{connect},
                       TranslatableString(strings::i18n::CONNECT))) {
                Settings::get().update_last_used_ip_address(
                    network_info->host_ip_address());
                network_info->lock_in_ip();

                // TODO so this saves the "last used" but maybe we want to
                // save the "last successful"
                Settings::get().write_save_file();
            }
        }

        // Back button
        {
            back = rect::rpad(back, 20);
            if (button(Widget{back},
                       TranslatableString(strings::i18n::BACK_BUTTON))) {
                network_info->unlock_username();
            }
        }

        // Even though the ui shows up at the top
        // we dont want the tabbing to be first, so
        // we put it here
        draw_username_with_edit(username, dt);
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
        if (MenuState::get().is_not(menu::State::Network)) return;

        ext::clear_background(ui::UI_THEME.background);

        using namespace ui;

        begin(ui_context, dt);
        draw_screen(dt);
        end();

        handle_announcements();
    }
};
