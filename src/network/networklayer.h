
#pragma once

#include "../external_include.h"
//
#include "../globals.h"
//
#include "../app.h"
#include "../layer.h"
#include "../ui.h"
//
#include "../player.h"
#include "../remote_player.h"
#include "network.h"

using namespace ui;

struct NetworkLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    std::shared_ptr<network::Info> network_info;
    std::map<int, std::shared_ptr<RemotePlayer>> remote_players;

    const SizeExpectation button_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};
    const SizeExpectation padd_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation padd_y = {.mode = Pixels, .value = 25.f};

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
        network_info->register_new_player_cb([&](int client_id) {
            if (remote_players.contains(client_id)) return;

            remote_players[client_id] = std::make_shared<RemotePlayer>();
            auto rp = remote_players[client_id];
            rp->client_id = client_id;
            EntityHelper::addEntity(remote_players[client_id]);
            std::cout << fmt::format("Adding a player {}", client_id)
                      << std::endl;
        });

        network_info->register_remove_player_cb([&](int client_id) {
            auto rp = remote_players[client_id];
            if (!rp)
                std::cout << fmt::format("doesnt exist but should {}",
                                         client_id)
                          << std::endl;
            rp->cleanup = true;
            remote_players.erase(client_id);
        });
        network_info->register_update_player_cb(
            [&](int client_id, std::string name, float* location, int facing) {
                auto rp = remote_players[client_id];
                if (!rp)
                    std::cout
                        << fmt::format("doesnt exist but should {}", client_id)
                        << std::endl;
                rp->update_remotely(name, location, facing);
            });

        network_info->register_player_packet_cb([&](int my_id) {
            Player me = GLOBALS.get<Player>("player");
            network::ClientPacket player({
                .client_id = my_id,
                .msg_type = network::ClientPacket::MsgType::PlayerLocation,
                .msg = network::ClientPacket::PlayerInfo({
                    .name = network_info->username,
                    .location =
                        {
                            me.position.x,
                            me.position.y,
                            me.position.z,
                        },
                    .facing_direction = static_cast<int>(me.face_direction),
                }),
            });
            return player;
        });
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

    void draw_base_screen() {
        auto left_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

        auto content = ui_context->own(Widget(
            {.mode = Children, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

        auto top_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto host_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto join_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto back_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto ping_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto play_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto cancel_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto button_padding = ui_context->own(Widget(padd_x, padd_y));

        auto bottom_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto connecting_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        padding(*left_padding);
        div(*content);
        ui_context->push_parent(content);
        {
            padding(*top_padding);
            text(*connecting_text,
                 fmt::format("Username: {}", network_info->username));
            padding(*button_padding);
            if (button(*host_button, "Host")) {
                network_info->set_role_to_host();
            }
            padding(*button_padding);
            if (button(*join_button, "Join")) {
                network_info->set_role_to_client();
            }
            padding(*button_padding);
            if (button(*back_button, "Back")) {
                Menu::get().state = Menu::State::Root;
            }
            padding(*bottom_padding);
        }
        ui_context->pop_parent();
    }

    void draw_ip_input_screen() {
        auto left_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

        auto content = ui_context->own(Widget(
            {.mode = Children, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

        auto top_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto submit_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto cancel_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto button_padding = ui_context->own(Widget(padd_x, padd_y));

        auto bottom_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        // TODO support validation
        auto ip_address_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID),
                   {.mode = Pixels, .value = 400.f, .strictness = 1.f},
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));
        auto ip_info_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        padding(*left_padding);
        div(*content);
        ui_context->push_parent(content);
        {
            padding(*top_padding);
            text(*player_text,
                 fmt::format("Username: {}", network_info->username));

            text(*ip_info_text, "Enter IP Address");
            textfield(*ip_address_input, network_info->host_ip_address);
            padding(*button_padding);
            if (button(*submit_button, "Connect")) {
                network_info->lock_in_ip();
            }
            padding(*button_padding);
            if (button(*cancel_button, "Back")) {
                network_info->username_set = false;
            }
            padding(*bottom_padding);
        }
        ui_context->pop_parent();
    }

    void draw_connected_screen() {
        auto left_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

        auto content = ui_context->own(Widget(
            {.mode = Children, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

        auto top_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto play_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto cancel_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto button_padding = ui_context->own(Widget(padd_x, padd_y));

        auto bottom_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto connecting_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        padding(*left_padding);
        div(*content);
        ui_context->push_parent(content);
        {
            padding(*top_padding);
            // TODO add button to edit as long as you arent currently
            // hosting people?
            // TODO add support for wstring
            text(*connecting_text,
                 fmt::format("Username: {}", network_info->username));

            for (auto kv : remote_players) {
                text(*player_text,
                     fmt::format("{}({})", kv.second->name, kv.first));
            }

            if (network_info->is_host()) {
                if (button(*play_button, "Play")) {
                    // TODO add a way to subscribe to state changes
                    network_info->send_updated_state(Menu::State::Game);
                }
            }

            if (button(*cancel_button, "Disconnect")) {
                network_info->set_role_to_none();
            }
            padding(*bottom_padding);
        }
        ui_context->pop_parent();
    }

    void draw_username_picker() {
        auto left_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

        auto content = ui_context->own(Widget(
            {.mode = Children, .strictness = 1.f},
            {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

        auto top_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto submit_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto cancel_button =
            ui_context->own(Widget(MK_UUID(id, ROOT_ID), button_x, button_y));
        auto button_padding = ui_context->own(Widget(padd_x, padd_y));

        auto bottom_padding = ui_context->own(
            Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                   {.mode = Percent, .value = 1.f, .strictness = 0.f}));

        auto username_input = ui_context->own(
            Widget(MK_UUID(id, ROOT_ID),
                   {.mode = Pixels, .value = 400.f, .strictness = 1.f},
                   {.mode = Pixels, .value = 25.f, .strictness = 0.5f}));

        auto player_text = ui_context->own(
            Widget({.mode = Pixels, .value = 120.f, .strictness = 0.5f},
                   {.mode = Pixels, .value = 100.f, .strictness = 1.f}));

        padding(*left_padding);
        div(*content);
        ui_context->push_parent(content);
        {
            padding(*top_padding);
            text(*player_text, "Username: ");
            textfield(*username_input, network_info->username);
            padding(*button_padding);
            if (button(*submit_button, "Lock in")) {
                network_info->username_set = true;
            }
            padding(*button_padding);
            if (button(*cancel_button, "Back")) {
                Menu::get().state = Menu::State::Root;
            }
            padding(*bottom_padding);
        }
        ui_context->pop_parent();
    }

    virtual void onDraw(float) override {
        draw_network_overlay();

        if (Menu::get().state != Menu::State::Network) return;

        if (minimized) {
            return;
        }
        ClearBackground(ui_context->active_theme().background);

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        auto root = ui_context->own(
            Widget({.mode = Pixels, .value = WIN_W, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f},
                   GrowFlags::Row));

        ui_context->push_parent(root);
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
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};
