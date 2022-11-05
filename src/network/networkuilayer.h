
#pragma once

#include "../external_include.h"
//
#include "../globals.h"
//
#include "../app.h"
#include "../layer.h"
#include "../settings.h"
#include "../ui.h"
//
#include "../player.h"
#include "../remote_player.h"
#include "network.h"
#include "raylib.h"
#include "webrequest.h"

using namespace ui;

struct NetworkUILayer : public Layer {
    network::Info* network_info;
    std::shared_ptr<network::Info>* ptr_to_shared_network_info;
    std::shared_ptr<ui::UIContext> ui_context;
    std::optional<std::string> my_ip_address;
    bool should_show_host_ip = false;

    const SizeExpectation icon_button_x = {.mode = Pixels, .value = 75.f};
    const SizeExpectation icon_button_y = {.mode = Pixels, .value = 25.f};
    const SizeExpectation button_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};
    const SizeExpectation padd_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation padd_y = {.mode = Pixels, .value = 25.f};

    NetworkUILayer() : Layer("Network") {
        minimized = false;
        ui_context.reset(new ui::UIContext());
        ptr_to_shared_network_info =
            GLOBALS.get_ptr<std::shared_ptr<network::Info>>(
                "network_info_shared_ptr");
        network_info = ptr_to_shared_network_info->get();
    }

    void reset_network_info() {
        std::shared_ptr<network::Info> shared_network_info =
            *ptr_to_shared_network_info;
        shared_network_info.reset(new network::Info());
    }

    virtual ~NetworkUILayer() {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &NetworkUILayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&NetworkUILayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &NetworkUILayer::onGamepadAxisMoved, this, std::placeholders::_1));
        dispatcher.dispatch<CharPressedEvent>(std::bind(
            &NetworkUILayer::onCharPressedEvent, this, std::placeholders::_1));
    }

    bool onCharPressedEvent(CharPressedEvent& event) {
        return ui_context.get()->process_char_press_event(event);
    }

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) {
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float) override {}

    std::shared_ptr<Widget> mk_button(uuid id) {
        return ui_context->own(Widget(id, button_x, button_y));
    }

    std::shared_ptr<Widget> mk_icon_button(uuid id) {
        return ui_context->own(Widget(id, icon_button_x, icon_button_y));
    }

    std::shared_ptr<Widget> mk_but_pad() {
        return ui_context->own(Widget(padd_x, padd_y));
    }

    std::shared_ptr<Widget> mk_text() {
        return ui_context->own(
            Widget({.mode = Pixels, .value = 275.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 50.f, .strictness = 1.f}));
    }

    void draw_username() {
        auto content =
            ui_context->own(Widget({.mode = Children, .strictness = 1.f},
                                   {.mode = Children, .strictness = 1.f}, Row));
        div(*content);
        ui_context->push_parent(content);
        {
            text(*mk_text(),
                 fmt::format("Username: {}", Settings::get().data.username));
            if (button(*mk_icon_button(MK_UUID(id, ROOT_ID)), "Edit")) {
                network_info->username_set = false;
            }
        }
        ui_context->pop_parent();
    }

    void draw_base_screen() {
        draw_username();
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Host")) {
            network_info->set_role_to_host();
        }
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Join")) {
            network_info->set_role_to_client();
        }
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Back")) {
            App::get().popLayer(this);
        }
    }

    void draw_ip_input_screen() {
        draw_username();
        // TODO support validation
        auto ip_address_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID),
                   {.mode = Pixels, .value = 400.f, .strictness = 1.f},
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));
        text(*mk_text(), "Enter IP Address");
        textfield(*ip_address_input, network_info->host_ip_address);
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Connect")) {
            // network_info->host_ip_address = "127.0.0.1";
            network_info->lock_in_ip();
        }
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Back")) {
            network_info->username_set = false;
        }
    }

    void draw_connected_screen() {
        if (network_info->is_host() && my_ip_address.has_value()) {
            auto content = ui_context->own(
                Widget({.mode = Children, .strictness = 1.f},
                       {.mode = Children, .strictness = 1.f}, Row));
            div(*content);
            ui_context->push_parent(content);
            {
                auto ip = should_show_host_ip ? my_ip_address.value()
                                              : "***.***.***.***";
                text(*mk_text(), fmt::format("Your IP is: {}", ip));
                auto checkbox_widget = ui_context->own(
                    Widget(MK_UUID(id, ROOT_ID),
                           {.mode = Pixels, .value = 75.f, .strictness = 0.5f},
                           {.mode = Pixels, .value = 25.f, .strictness = 1.f}));
                std::string show_hide_host_ip_text =
                    should_show_host_ip ? "Hide" : "Show";
                if (checkbox(*checkbox_widget, &should_show_host_ip,
                             &show_hide_host_ip_text)) {
                }
                if (button(*mk_icon_button(MK_UUID(id, ROOT_ID)), "Copy")) {
                    SetClipboardText(my_ip_address.value().c_str());
                }
            }
            ui_context->pop_parent();
        }
        draw_username();

        // TODO add button to edit as long as you arent currently
        // hosting people?
        for (auto kv : network_info->remote_players) {
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
            if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Play")) {
                network_info->send_updated_state(Menu::State::Game);
            }
        }

        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Disconnect")) {
            reset_network_info();
        }
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
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Lock in")) {
            network_info->username_set = true;
        }
        padding(*mk_but_pad());
        if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Back")) {
            App::get().popLayer(this);
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
        ClearBackground(ui_context->active_theme().background);

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);

        auto root = ui_context->own(
            Widget({.mode = Pixels, .value = WIN_W, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f},
                   GrowFlags::Row));

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
