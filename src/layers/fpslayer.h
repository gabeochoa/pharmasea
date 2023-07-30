#pragma once

#include "../engine/globals_register.h"
#include "../engine/layer.h"
#include "../engine/profile.h"
#include "../external_include.h"
#include "../preload.h"
#include "raylib.h"

namespace network {
    extern long long total_ping;
    extern long long there_ping;
    extern long long return_ping;
}

using namespace profile;
struct FPSLayer : public Layer {
    FPSLayer() : Layer(strings::menu::FPS) { Profiler::clear(); }

    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        ext::draw_fps(0, 0);

        const std::string str =
            fmt::format("ping {}ms (to{},from{})", network::total_ping,
                        network::there_ping, network::return_ping);
        DrawTextEx(Preload::get().font, str.c_str(), vec2{75, 0}, 20, 0, WHITE);

        if (GLOBALS.get<bool>("debug_ui_enabled")) {
            std::vector<Sample> pairs;
            pairs.insert(pairs.end(), Profiler::get()._acc.begin(),
                         Profiler::get()._acc.end());

            sort(pairs.begin(), pairs.end(),
                 [](const Sample& a, const Sample& b) {
                     return a.second.average() > b.second.average();
                 });

            int ypos = 0;
            int spacing = 20;
            for (const auto& kv : pairs) {
                auto stats = kv.second;
                std::string stat_str =
                    fmt::format("{}{}: avg {:.0f}ns", stats.filename, kv.first,
                                stats.average());
                int string_width = raylib::MeasureText(stat_str.c_str(), 15);
                DrawRectangle(100, ypos, string_width, 20, BLACK);
                DrawTextEx(Preload::get().font, stat_str.c_str(),
                           vec2{100, (float) ypos}, 20, 0, WHITE);
                ypos += spacing;
            }
        }
    }
};
