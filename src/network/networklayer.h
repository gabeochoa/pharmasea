
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
#include "../player.h"
#include "../remote_player.h"
#include "network.h"
#include "raylib.h"
#include "webrequest.h"

using namespace ui;

struct NetworkLayer : public Layer {
    std::shared_ptr<network::Info> network_info;
    std::optional<std::string> my_ip_address;

    NetworkLayer() : Layer("Network") {
        network::Info::init_connections();
        network_info.reset(new network::Info());
        my_ip_address = network::get_remote_ip_address();
        if (!Settings::get().data.username.empty()) {
            network_info->username_set = true;
        }

        // NOTE: this is weird but due to how I set this up originally,
        // yes i mean to add the ptr to the smart_ptr object and not
        // the underlying one
        GLOBALS.set("network_info_shared_ptr", &(network_info));
    }

    virtual ~NetworkLayer() { network::Info::shutdown_connections(); }
    virtual void onEvent(Event&) override {}
    virtual void onUpdate(float dt) override { network_info->tick(dt); }
    virtual void onDraw(float) override {}
};
