
#pragma once

#include "../external_include.h"
//
#include "../globals.h"
#include "../globals_register.h"
//
#include "../app.h"
#include "../layer.h"
#include "../settings.h"
#include "../ui.h"
//
#include "../gamelayer.h"
#include "../player.h"
#include "../remote_player.h"
#include "network.h"
#include "raylib.h"
#include "webrequest.h"

using namespace ui;

struct NetworkLayer : public Layer {
    std::optional<std::string> my_ip_address;

    NetworkLayer() : Layer("Network") {
        network::Info::init_connections();
        my_ip_address = network::get_remote_ip_address();
        if (!Settings::get().data.username.empty()) {
            network::Info::get().username_set = true;
        }
    }

    virtual ~NetworkLayer() { network::Info::shutdown_connections(); }

    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float dt) override {
        network::Info::get().tick(dt);
        handle_updated_lobby_state();
    }

    void handle_updated_lobby_state() {
        if (network::Info::get().processed_updated_lobby_state) return;

        switch (network::Info::get().network_lobby_state) {
            case network::LobbyState::Game:
                App::get().pushLayer(new GameLayer());
                break;
            case network::LobbyState::Paused:
                App::get().pushLayer(new PauseLayer());
                break;
            case network::LobbyState::Lobby:
                App::get().pushLayer(new NetworkLayer());
                break;
        }

        network::Info::get().processed_updated_lobby_state = true;
    }
    virtual void onDraw(float) override {}
};
