#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "../building_locations.h"
#include "../camera.h"
#include "../engine/app.h"
#include "../engine/layer.h"
#include "../engine/runtime_globals.h"
#include "../engine/shader_library.h"
#include "../external_include.h"
#include "../globals.h"
#include "../lighting_runtime.h"
#include "../map.h"
#include "../map_generation/pipeline.h"
#include "ah.h"

// For EXAMPLE_MAP
extern std::vector<std::string> EXAMPLE_MAP;

// A debug layer for visualizing generated maps without running the full game.
// Launched via: ./pharmasea.exe --map-viewer --seed=xyz
struct MapViewerLayer : public Layer {
    std::unique_ptr<GameCam> cam;
    std::unique_ptr<Map> map;
    raylib::RenderTexture2D render_texture;
    std::string seed;
    mapgen::BarArchetype archetype;

    // Camera movement state
    vec3 camera_target = {10.f, 0.f, 10.f};
    float camera_distance = 25.f;
    float camera_yaw = 0.f;
    float camera_pitch = -45.f * DEG2RAD;

    explicit MapViewerLayer(const std::string& map_seed)
        : Layer("MapViewer"),
          cam(std::make_unique<GameCam>()),
          seed(map_seed) {
        globals::set_game_cam(cam.get());
        render_texture = raylib::LoadRenderTexture(WIN_W(), WIN_H());

        std::cout << "\n=== Map Viewer ===" << std::endl;
        std::cout << "Seed: " << seed << std::endl;

        if (seed == "default_seed") {
            // Use DEFAULT_MAP from settings.json
            std::cout << "Using DEFAULT_MAP from settings.json" << std::endl;
            std::cout << "Size: " << EXAMPLE_MAP.size() << " lines" << std::endl;
            std::cout << "\nASCII Map:" << std::endl;
            for (const std::string& line : EXAMPLE_MAP) {
                std::cout << line << std::endl;
            }
        } else {
            // Generate and print ASCII map to console
            mapgen::GenerationContext ctx;
            mapgen::GeneratedAscii ascii = mapgen::generate_ascii(seed, ctx);
            archetype = ascii.archetype;

            std::cout << "Archetype: " << archetype_to_string(archetype)
                      << std::endl;
            std::cout << "Size: " << ctx.rows << "x" << ctx.cols << std::endl;
            std::cout << "\nASCII Map:" << std::endl;
            for (const std::string& line : ascii.lines) {
                std::cout << line << std::endl;
            }
        }
        std::cout << "\nControls: WASD=pan, QE=rotate, Mouse wheel=zoom, ESC=exit"
                  << std::endl;

        // Create the map with entities
        map = std::make_unique<Map>(seed);
        globals::set_world_map(map.get());
        map->ensure_generated_map(seed);

        // Center camera on BAR_BUILDING for all maps
        vec2 bar_center = BAR_BUILDING.center();
        camera_target = {bar_center.x, 0.f, bar_center.y};
    }

    virtual ~MapViewerLayer() {
        globals::set_world_map(nullptr);
        globals::set_game_cam(nullptr);
        raylib::UnloadRenderTexture(render_texture);
    }

    static const char* archetype_to_string(mapgen::BarArchetype a) {
        switch (a) {
            case mapgen::BarArchetype::OpenHall:
                return "OpenHall";
            case mapgen::BarArchetype::MultiRoom:
                return "MultiRoom";
            case mapgen::BarArchetype::BackRoom:
                return "BackRoom";
            case mapgen::BarArchetype::LoopRing:
                return "LoopRing";
        }
        return "Unknown";
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (event.keycode == raylib::KEY_ESCAPE) {
            App::get().close();
            return true;
        }
        return false;
    }

    virtual void onUpdate(float dt) override {
        raylib::SetExitKey(raylib::KEY_NULL);

        // Camera controls
        const float pan_speed = 10.f * dt;
        const float rotate_speed = 2.f * dt;
        const float zoom_speed = 2.f;

        // WASD for panning
        if (raylib::IsKeyDown(raylib::KEY_W)) camera_target.z -= pan_speed;
        if (raylib::IsKeyDown(raylib::KEY_S)) camera_target.z += pan_speed;
        if (raylib::IsKeyDown(raylib::KEY_A)) camera_target.x -= pan_speed;
        if (raylib::IsKeyDown(raylib::KEY_D)) camera_target.x += pan_speed;

        // QE for rotation
        if (raylib::IsKeyDown(raylib::KEY_Q)) camera_yaw -= rotate_speed;
        if (raylib::IsKeyDown(raylib::KEY_E)) camera_yaw += rotate_speed;

        // Mouse wheel for zoom
        float wheel = ext::get_mouse_wheel_move();
        camera_distance -= wheel * zoom_speed;
        camera_distance =
            std::clamp(camera_distance, 5.f, 50.f);

        // Update camera position based on orbit controls
        float cos_pitch = cosf(camera_pitch);
        float sin_pitch = sinf(camera_pitch);
        float cos_yaw = cosf(camera_yaw);
        float sin_yaw = sinf(camera_yaw);

        cam->camera.target = camera_target;
        cam->camera.position = {
            camera_target.x + camera_distance * cos_pitch * sin_yaw,
            camera_target.y + camera_distance * -sin_pitch,
            camera_target.z + camera_distance * cos_pitch * cos_yaw,
        };
    }

    virtual void onDraw(float dt) override {
        ext::clear_background(Color{100, 100, 100, 255});

        // Shader-based lighting
        const bool lighting_enabled = Settings::get().data.enable_lighting;
        raylib::Shader* lighting_shader = nullptr;
        if (lighting_enabled) {
            lighting_shader = &ShaderLibrary::get().get("lighting");
        }

        raylib::BeginMode3D(cam->get());
        {
            if (lighting_shader) {
                update_lighting_shader(*lighting_shader, cam->get());
                raylib::BeginShaderMode(*lighting_shader);
            }

            // Draw ground plane
            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){256.0f, 256.0f}, DARKGRAY);

            // Draw map entities
            if (map) {
                map->onDraw(dt);
            }

            if (lighting_shader) {
                raylib::EndShaderMode();
            }
        }
        raylib::EndMode3D();

        // Draw HUD info
        raylib::DrawText(
            fmt::format("Seed: {} | Archetype: {}", seed,
                        archetype_to_string(archetype))
                .c_str(),
            10, 10, 20, WHITE);
        raylib::DrawText("WASD=pan, QE=rotate, Scroll=zoom, ESC=exit", 10, 35,
                         16, LIGHTGRAY);
    }
};

