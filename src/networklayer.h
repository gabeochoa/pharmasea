
#pragma once

#include "external_include.h"
//
#include "globals.h"
//
#include "app.h"
#include "layer.h"
#include "settings.h"
#include "ui.h"
//
#include "network/network.h"
#include "network/webrequest.h"
#include "player.h"
#include "raylib.h"
#include "remote_player.h"
#include "ui_context.h"

using namespace ui;

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    std::shared_ptr<network::Info> network_info;
    std::optional<std::string> my_ip_address;
    bool should_show_host_ip = false;

    NetworkLayer() : Layer("Network") {
        ui_context.reset(new ui::UIContext());

        network::Info::init_connections();
        network_info.reset(new network::Info());
        my_ip_address = network::get_remote_ip_address();
        if (!Settings::get().data.username.empty()) {
            network_info->lock_in_username();
        }
    }

    virtual ~NetworkLayer() { network::Info::shutdown_connections(); }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &NetworkLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&NetworkLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &NetworkLayer::onGamepadAxisMoved, this, std::placeholders::_1));
        dispatcher.dispatch<CharPressedEvent>(std::bind(
            &NetworkLayer::onCharPressedEvent, this, std::placeholders::_1));
    }

    bool onCharPressedEvent(CharPressedEvent& event) {
        if (Menu::get().state != Menu::State::Network) return false;
        return ui_context.get()->process_char_press_event(event);
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) {
        if (Menu::get().state != Menu::State::Network) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::Network) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (Menu::get().state != Menu::State::Network) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float dt) override {
        // NOTE: this has to go above the checks since it always has to run
        network_info->tick(dt);

        if (Menu::get().state != Menu::State::Network) return;
        // if we get here, then user clicked "join"
    }

    void draw_network_overlay() {}

    void draw_username() {
        auto content = ui::components::mk_row();
        div(*content);
        ui_context->push_parent(content);
        {
            text(*ui::components::mk_text(),
                 fmt::format("Username: {}", Settings::get().data.username));
            if (button(*ui::components::mk_icon_button(MK_UUID(id, ROOT_ID)),
                       "Edit")) {
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
                           "Host")) {
                    network_info->set_role(network::Info::Role::s_Host);
                }
                padding(*ui::components::mk_but_pad());
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           "Join")) {
                    network_info->set_role(network::Info::Role::s_Client);
                }
                padding(*ui::components::mk_but_pad());
                if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                           "Back")) {
                    Menu::get().state = Menu::State::Root;
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
        // TODO support validation
        auto ip_address_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID),
                   {.mode = Pixels, .value = 400.f, .strictness = 1.f},
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));
        text(*ui::components::mk_text(), "Enter IP Address");
        textfield(*ip_address_input, network_info->host_ip_address());
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   "Connect")) {
            network_info->lock_in_ip();
        }
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)), "Back")) {
            network_info->unlock_username();
        }
    }

    void draw_connected_screen() {
        auto draw_host_network_info = [&]() {
            if (network_info->is_host() && my_ip_address.has_value()) {
                auto content = ui_context->own(
                    Widget({.mode = Children, .strictness = 1.f},
                           {.mode = Children, .strictness = 1.f}, Row));
                div(*content);
                ui_context->push_parent(content);
                {
                    auto ip = should_show_host_ip ? my_ip_address.value()
                                                  : "***.***.***.***";
                    text(*ui::components::mk_text(),
                         fmt::format("Your IP is: {}", ip));
                    auto checkbox_widget = ui_context->own(Widget(
                        MK_UUID(id, ROOT_ID),
                        {.mode = Pixels, .value = 75.f, .strictness = 0.5f},
                        {.mode = Pixels, .value = 25.f, .strictness = 1.f}));
                    std::string show_hide_host_ip_text =
                        should_show_host_ip ? "Hide" : "Show";
                    if (checkbox(*checkbox_widget, &should_show_host_ip,
                                 &show_hide_host_ip_text)) {
                    }
                    if (button(*ui::components::mk_icon_button(
                                   MK_UUID(id, ROOT_ID)),
                               "Copy")) {
                        SetClipboardText(my_ip_address.value().c_str());
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
                       {.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                       {.mode = Pixels, .value = 100.f, .strictness = 1.f}));
            text(*player_text,
                 fmt::format("{}({})", kv.second->name, kv.first));
        }

        if (network_info->is_host()) {
            if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                       "Start")) {
                Menu::get().state = Menu::State::Game;
            }
            padding(*ui::components::mk_but_pad());
        }

        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   "Disconnect")) {
            network_info.reset(new network::Info());
        }

        padding(*ui::components::mk_but_pad());

        draw_host_network_info();
    }

    void draw_username_picker() {
        auto username_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID),
                   {.mode = Pixels, .value = 400.f, .strictness = 1.f},
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        text(*player_text, "Username: ");
        textfield(*username_input, Settings::get().data.username,
                  network::MAX_NAME_LENGTH);
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)),
                   "Lock in")) {
            network_info->lock_in_username();
        }
        padding(*ui::components::mk_but_pad());
        if (button(*ui::components::mk_button(MK_UUID(id, ROOT_ID)), "Back")) {
            Menu::get().state = Menu::State::Root;
        }
    }

    void draw_screen_selector_logic() {
        if (!network_info->username_set) {
            draw_username_picker();
        } else if (network_info->has_role()) {
            if (!(network_info->has_set_ip())) {
                draw_ip_input_screen();
            } else {
                draw_connected_screen();
            }
        } else {
            draw_base_screen();
        }
    }

    virtual void onDraw(float dt) override {
        draw_network_overlay();

        if (Menu::get().state != Menu::State::Network) return;

        if (minimized) {
            return;
        }
        ClearBackground(ui_context->active_theme().background);

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);

        auto root = ui::components::mk_root();

        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                       {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

            auto content = ui_context->own(Widget(
                {.mode = Children, .strictness = 1.f},
                {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 1.f, .strictness = 0.f}));
                padding(*top_padding);
                {  //
                    draw_screen_selector_logic();
                }
                padding(*ui_context->own(Widget(
                    {.mode = Pixels, .value = 100.f, .strictness = 1.f},
                    {.mode = Percent, .value = 1.f, .strictness = 0.f})));
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};
