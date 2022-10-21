
#pragma once

#include "app.h"
#include "external_include.h"
#include "globals.h"
#include "layer.h"
#include "network.h"
#include "ui.h"

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    std::shared_ptr<network::Info> network_info;

    NetworkLayer() : Layer("Network") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context->init();
        ui_context->set_font(Preload::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);

        network_info.reset(new network::Info());
    }

    virtual ~NetworkLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}
    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &NetworkLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&NetworkLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(std::bind(
            &NetworkLayer::onGamepadAxisMoved, this, std::placeholders::_1));
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

    void process_network_stuff(float dt) {
        if (network_info->is_host()) {
            network_info->server_tick(dt, 10);
        } else {
            network_info->client_tick(dt, 10);
        }
    }

    virtual void onUpdate(float dt) override {
        // NOTE: this has to go above the checks since it always has to run
        process_network_stuff(dt);

        if (Menu::get().state != Menu::State::Network) return;
        // if we get here, then user clicked "join"
    }

    void draw_ui() {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        const SizeExpectation padd_x = {
            .mode = Pixels, .value = 120.f, .strictness = 0.9f};
        const SizeExpectation padd_y = {
            .mode = Pixels, .value = 25.f, .strictness = 0.5f};

        ui_context->begin(mouseDown, mousepos);

        ui::Widget root({.mode = Pixels, .value = WIN_W, .strictness = 1.f},
                        {.mode = Pixels, .value = WIN_H, .strictness = 1.f},
                        GrowFlags::Row);

        Widget left_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = WIN_H, .strictness = 1.f});

        Widget content({.mode = Children, .strictness = 1.f},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f},
                       Column);

        Widget top_padding({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 1.f, .strictness = 0.f});

        const SizeExpectation button_x = {.mode = Pixels, .value = 120.f};
        const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};
        Widget host_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget join_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget back_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget ping_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget cancel_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget button_padding(padd_x, padd_y);

        Widget bottom_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 0.f});

        Widget connecting_text(
            {.mode = Pixels, .value = 120.f, .strictness = 0.5f},
            {.mode = Pixels, .value = 100.f, .strictness = 1.f});

        ui_context->push_parent(&root);
        {
            padding(left_padding);
            div(content);

            ui_context->push_parent(&content);
            {
                padding(top_padding);
                text(connecting_text, network_info->status());
                if (network_info->is_host() || network_info->is_client()) {
                    if(network_info->is_client()){
                    if (button(ping_button, "Ping")) {
                        network::client::send(network_info->client_info);
                    }

                    }
                    if (button(cancel_button, "Cancel")) {
                        network_info->set_role_to_none();
                    }
                } else {
                    padding(button_padding);
                    if (button(host_button, "Host")) {
                        network_info->set_role_to_host();
                    }
                    padding(button_padding);
                    if (button(join_button, "Join")) {
                        network_info->set_role_to_client();
                    }
                    padding(button_padding);
                    if (button(back_button, "Back")) {
                        network_info->set_role_to_none();
                        Menu::get().state = Menu::State::Root;
                    }
                }
                padding(bottom_padding);
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(&root);
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Network) return;

        if (minimized) {
            return;
        }
        ClearBackground(ui_context->active_theme().background);

        draw_ui();

        DrawTextEx(Preload::get().font, network_info->status().c_str(), {5, 50},
                   20, 0, LIGHTGRAY);
    }
};
