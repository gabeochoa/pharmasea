#pragma once

#include "../engine.h"
#include "../engine/layer.h"
#include "../external_include.h"
#include "../globals.h"
#include "../preload.h"

namespace network {
extern long long total_ping;
extern long long there_ping;
extern long long return_ping;
}  // namespace network

struct VersionLayer : public Layer {
    VersionLayer() : Layer("Version") {}
    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        if (MenuState::get().is(menu::State::Game)) {
            const std::string ping_str = network::LOCAL_ONLY
                                             ? std::string("ping local")
                                             : fmt::format(
                                                   "ping {}ms (to{},from{})",
                                                   network::total_ping,
                                                   network::there_ping,
                                                   network::return_ping);

            DrawTextEx(Preload::get().font, ping_str.c_str(),
                       {WIN_WF() - 225, 50}, 20, 0, WHITE);
        }

        DrawTextEx(Preload::get().font, VERSION.data(), {WIN_WF() - 200, 20},
                   20, 0, WHITE);
    }
};
