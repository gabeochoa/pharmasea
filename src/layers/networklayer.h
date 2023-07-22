
#pragma once

#include <regex>

#include "../engine.h"
#include "../engine/network/webrequest.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../engine/toastmanager.h"
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

    void draw_username() {
        auto content = ui::components::mk_row();
        div(*content);
        ui_context->push_parent(content);
        {
            text(*ui::components::mk_text(),
                 fmt::format("{}: {}", text_lookup(strings::i18n::USERNAME),
                             Settings::get().data.username));
            if (button(*ui::components::mk_icon_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::EDIT))) {
                network_info->unlock_username();
            }
        }
        ui_context->pop_parent();
    }

    void draw_base_screen() {
        auto content = ui::components::mk_row();
        div(*content);
        ui_context->push_parent(content);
        {
            auto col1 = ui::components::mk_column();
            div(*col1);
            ui_context->push_parent(col1);
            {
                padding(*ui::components::mk_but_pad());
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           text_lookup(strings::i18n::HOST))) {
                    network_info->set_role(network::Info::Role::s_Host);
                }
                padding(*ui::components::mk_but_pad());
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           text_lookup(strings::i18n::JOIN))) {
                    network_info->set_role(network::Info::Role::s_Client);
                }
                padding(*ui::components::mk_but_pad());
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           text_lookup(strings::i18n::BACK_BUTTON))) {
                    MenuState::get().clear_history();
                    MenuState::get().set(menu::State::Root);
                }
            }
            ui_context->pop_parent();

            padding(*ui::components::mk_but_pad());

            auto col2 = ui::components::mk_column();
            div(*col2);
            ui_context->push_parent(col2);
            {
                //
                draw_username();
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
    }

    void draw_ip_input_screen() {
        draw_username();
        // TODO add show/hide button for the ip address
        auto ip_address_input = ui_context->own(Widget(
            MK_UUID(id, ROOT_ID), Size_Px(400.f, 1.f), Size_Px(25.f, 0.5f)));
        text(*ui::components::mk_text(), text_lookup(strings::i18n::ENTER_IP));
        // TODO add trimming of whitespace or validate whitespace
        textfield(*ip_address_input, network_info->host_ip_address(),
                  [](const std::string& content) {
                      // xxx.xxx.xxx.xxx
                      if (content.size() > 15) {
                          return TextfieldValidationDecisionFlag::StopNewInput;
                      }
                      if (validate_ip(content)) {
                          return TextfieldValidationDecisionFlag::Valid;
                      }
                      return TextfieldValidationDecisionFlag::Invalid;
                  });
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::LOAD_LAST_IP))) {
            network_info->host_ip_address() = Settings::get().last_used_ip();
        }
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::CONNECT))) {
            Settings::get().update_last_used_ip_address(
                network_info->host_ip_address());
            network_info->lock_in_ip();
        }
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::BACK_BUTTON))) {
            network_info->unlock_username();
        }
    }

    void draw_connected_screen() {
        auto draw_host_network_info = [&]() {
            if (network_info->is_host()) {
                auto content = ui_context->own(
                    Widget({.mode = Children, .strictness = 1.f},
                           {.mode = Children, .strictness = 1.f}, Row));
                div(*content);
                ui_context->push_parent(content);
                {
                    auto ip =
                        should_show_host_ip ? my_ip_address : "***.***.***.***";
                    text(*ui::components::mk_text(),
                         fmt::format("Your IP is: {}", ip));
                    auto checkbox_widget = ui_context->own(
                        Widget(MK_UUID(id, ROOT_ID), Size_Px(75.f, 0.5f),
                               Size_Px(25.f, 1.f)));
                    std::string show_hide_host_ip_text =
                        should_show_host_ip
                            ? text_lookup(strings::i18n::HIDE_IP)
                            : text_lookup(strings::i18n::SHOW_IP);
                    if (checkbox(*checkbox_widget, &should_show_host_ip,
                                 &show_hide_host_ip_text)) {
                    }
                    if (button(*ui::components::mk_icon_button(
                                   MK_UUID(id, ROOT_ID)),
                               text_lookup(strings::i18n::COPY_IP))) {
                        ext::set_clipboard_text(my_ip_address.c_str());
                    }
                }
                ui_context->pop_parent();
            }
            // TODO add button to edit as long as you arent currently
            // hosting people?
            draw_username();
        };

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

        if (network_info->is_host()) {
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       text_lookup(strings::i18n::START))) {
                MenuState::get().set(menu::State::Game);
                GameState::get().set(game::State::Lobby);
            }
            padding(*ui::components::mk_but_pad());
        }

        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::DISCONNECT))) {
            network_info.reset(new network::Info());
        }

        padding(*ui::components::mk_but_pad());

        draw_host_network_info();
    }

    void draw_username_picker() {
        auto username_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID), Size_Px(400.f, 1.f),
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   Size_Px(100.f, 1.f)));

        text(*player_text, text_lookup(strings::i18n::USERNAME));
        // TODO theres a problem where it is constantly saving as you type which
        // might not be expected
        textfield(*username_input, Settings::get().data.username,
                  // TODO probably make a "username validation" function
                  [](const std::string& content) {
                      if (content.size() >= network::MAX_NAME_LENGTH) {
                          return TextfieldValidationDecisionFlag::StopNewInput;
                      }
                      return TextfieldValidationDecisionFlag::Valid;
                  });
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::LOCK_IN))) {
            network_info->lock_in_username();
        }
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   text_lookup(strings::i18n::BACK_BUTTON))) {
            MenuState::get().go_back();
        }
    }

    void draw_screen_selector_logic() {
        if (network_info->missing_username()) {
            draw_username_picker();
            return;
        }

        if (network_info->missing_role()) {
            draw_base_screen();
            return;
        }

        if (network_info->has_set_ip()) {
            draw_connected_screen();
            return;
        }
        draw_ip_input_screen();
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

    virtual void onDraw(float dt) override {
        // TODO add an overlay that shows who's currently available
        // draw_network_overlay();

        if (MenuState::get().is_not(menu::State::Network)) return;

        ui_context->begin(dt);

        ClearBackground(ui_context->active_theme().background);

        auto root = ui::components::mk_root();
        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget(Size_Px(100.f, 1.f), Size_Px(WIN_HF(), 1.f)));

            auto content = ui_context->own(Widget(
                {.mode = Children, .strictness = 1.f},
                {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget(Size_Px(100.f, 1.f), Size_Pct(1.f, 0.f)));
                padding(*top_padding);
                {  //
                    draw_screen_selector_logic();
                    handle_announcements();
                }
                padding(*ui_context->own(
                    Widget(Size_Px(100.f, 1.f), Size_Pct(1.f, 0.f))));
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};
