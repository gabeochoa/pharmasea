
#include "gamelayer.h"

#include "../building_locations.h"
#include "../drawing_util.h"
#include "../engine/ui/color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../components/transform.h"
#include "../engine.h"
#include "../engine/layer.h"
#include "../engine/settings.h"
#include "../engine/shader_library.h"
#include "../entity_query.h"
#include "../entity_type.h"
#include "../lighting_runtime.h"
#include "../map.h"
#include "../system/core/system_manager.h"
#include "raylib.h"
GameLayer::~GameLayer() { raylib::UnloadRenderTexture(game_render_texture); }

bool GameLayer::onGamepadAxisMoved(GamepadAxisMovedEvent&) { return false; }

bool GameLayer::onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
    if (!MenuState::s_in_game()) return false;
    if (KeyMap::get_button(menu::State::Game, InputName::Pause) ==
        event.button) {
        GameState::get().pause();
        return true;
    }
    return false;
}

bool GameLayer::onKeyPressed(KeyPressedEvent& event) {
    if (!MenuState::s_in_game()) return false;
    // Note: You can only pause in game state, in planning no pause
    if (KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
        event.keycode) {
        GameState::get().pause();
        //  TODO obv need to have this fun on a timer or something instead
        //  of on esc
        // SoundLibrary::get().play_random_match("pa_announcements_");
        return true;
    }
    return false;
}

bool GameLayer::onMouseButtonUp(Mouse::MouseButtonUpEvent& event) {
    if (!MenuState::s_in_game()) return false;
    // TODO remove protected ew
    // TODO better button naming
    if (event.GetMouseButton() == Mouse::MouseCode::Button0) {
        raylib::ShowCursor();
    }
    return false;
}

bool GameLayer::onMouseButtonDown(Mouse::MouseButtonDownEvent& event) {
    if (!MenuState::s_in_game()) return false;
    // TODO remove protected ew
    // TODO better button naming
    if (event.GetMouseButton() == Mouse::MouseCode::Button0) {
        raylib::HideCursor();
    }
    return false;
}

void GameLayer::play_music() {
    if (ENABLE_SOUND) {
        auto m = MusicLibrary::get().get(strings::music::SUPERMARKET);
        if (!IsMusicStreamPlaying(m)) {
            PlayMusicStream(m);
        }
        UpdateMusicStream(m);
    }
}

void GameLayer::onUpdate(float dt) {
    TRACY_ZONE_SCOPED;
    if (MenuState::s_in_game()) play_music();

    if (!GameState::get().should_update()) return;

    // Dont quit window on escape
    raylib::SetExitKey(raylib::KEY_NULL);

    auto* act = globals::active_camera_target();
    if (!act) {
        return;
    }

    cam->updateToTarget(act->get<Transform>().pos());
    cam->updateCamera();

    //         jun 24-23 we need this so furniture shows up
    auto* map_ptr = globals::world_map();
    if (map_ptr) {
        // NOTE: today we need to grab things so that the client renders
        // what they server has access to
        EntityHelper::cleanup();
        map_ptr->onUpdateLocalPlayers(dt);
        map_ptr->onUpdateRemotePlayers(dt);
    }
}

void draw_building(const Building& building) {
    Color f = Color{0, 250, 50, 15};
    Color b = Color{250, 0, 50, 15};

    DrawCubeCustom(building.to3(), building.area.width, 1, building.area.height,
                   0, f, b);
}

void GameLayer::draw_world(float dt) {
    auto* map_ptr = globals::world_map();
    const auto network_debug_mode_on = globals::network_ui_enabled();
    if (network_debug_mode_on) {
        // NOTE(threading): The authoritative server map is mutated on the
        // dedicated server thread. Rendering it directly on the main thread is
        // a data race. If we want server-side debug rendering, publish a
        // snapshot from the server thread instead.
        // TODO(threading): Add a server->main snapshot for safe debug draw.
    }

    // Shader-based lighting (Half-Lambert + Blinn-Phong).
    const bool lighting_enabled = Settings::get().data.enable_lighting;
    raylib::Shader* lighting_shader = nullptr;
    if (lighting_enabled) {
        lighting_shader = &ShaderLibrary::get().get("lighting");
    }

    raylib::BeginMode3D((*cam).get());
    {
        if (lighting_shader) {
            update_lighting_shader(*lighting_shader, (*cam).get());
            raylib::BeginShaderMode(*lighting_shader);
        }

        raylib::DrawPlane((vec3) {0.0f, -TILESIZE, 0.0f},
                          (vec2) {256.0f, 256.0f}, DARKGRAY);
        if (map_ptr) map_ptr->onDraw(dt);

        if (lighting_shader) {
            raylib::EndShaderMode();
        }

        if (globals::debug_ui_enabled()) {
            draw_building(LOBBY_BUILDING);
            draw_building(MODEL_TEST_BUILDING);
            draw_building(PROGRESSION_BUILDING);
            draw_building(STORE_BUILDING);
            draw_building(BAR_BUILDING);
            draw_building(LOAD_SAVE_BUILDING);
        }

        // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        // if (nav) {
        // for (auto kv : nav->entityShapes) {
        // DrawLineStrip2Din3D(kv.second.hull, PINK);
        // }
        //
        // for (auto kv : nav->shapes) {
        // DrawLineStrip2Din3D(kv.hull, PINK);
        // }
        // }
    }
    raylib::EndMode3D();
}

void GameLayer::onDraw(float dt) {
    TRACY_ZONE_SCOPED;
    if (!MenuState::s_in_game()) return;

    auto* map_ptr = globals::world_map();

    ext::clear_background(Color{200, 200, 200, 255});
    draw_world(dt);

    // note: for ui stuff
    if (map_ptr) map_ptr->onDrawUI(dt);
}
