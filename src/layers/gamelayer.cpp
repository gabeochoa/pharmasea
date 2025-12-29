
#include "gamelayer.h"

#include "../building_locations.h"
#include "../drawing_util.h"
#include "../engine/ui/color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../engine.h"
#include "../engine/layer.h"
#include "../map.h"
#include "../system/system_manager.h"
#include "raylib.h"

namespace {

inline vec2 world_to_screen(const vec3& world, const raylib::Camera3D& cam) {
    return raylib::GetWorldToScreen(world, cam);
}

inline void draw_screen_quad(const vec2& p0, const vec2& p1, const vec2& p2,
                             const vec2& p3, Color col) {
    raylib::DrawTriangle(p0, p1, p2, col);
    raylib::DrawTriangle(p0, p2, p3, col);
}

void draw_projected_building_rect_outline(const Building& b,
                                          const raylib::Camera3D& cam, float y,
                                          Color col) {
    const float x0 = b.area.x;
    const float z0 = b.area.y;
    const float x1 = b.area.x + b.area.width;
    const float z1 = b.area.y + b.area.height;

    const vec2 p0 = world_to_screen({x0, y, z0}, cam);
    const vec2 p1 = world_to_screen({x1, y, z0}, cam);
    const vec2 p2 = world_to_screen({x1, y, z1}, cam);
    const vec2 p3 = world_to_screen({x0, y, z1}, cam);

    // Outline
    raylib::DrawLine((int) p0.x, (int) p0.y, (int) p1.x, (int) p1.y, col);
    raylib::DrawLine((int) p1.x, (int) p1.y, (int) p2.x, (int) p2.y, col);
    raylib::DrawLine((int) p2.x, (int) p2.y, (int) p3.x, (int) p3.y, col);
    raylib::DrawLine((int) p3.x, (int) p3.y, (int) p0.x, (int) p0.y, col);
}

struct Phase1LightingTuning {
    // Night ambience: alpha-black tint over whole screen.
    unsigned char night_outdoor_dark_alpha = 190;
    // Indoor lift: additive warm fill over roof rectangles.
    Color night_indoor_lift = Color{60, 50, 35, 120};
    // Optional: day tint (kept subtle; can be turned off by setting alpha=0).
    Color day_tint = Color{255, 240, 220, 10};
    // Projection height used for building masks.
    float mask_y = -TILESIZE / 2.f;
};

static const Phase1LightingTuning PHASE1{};

void draw_phase1_lighting_overlay(const GameCam& game_cam) {
    const bool enabled =
        GLOBALS.get_or_default<bool>("lighting_debug_enabled", false);
    if (!enabled) return;

    const bool overlay_only =
        GLOBALS.get_or_default<bool>("lighting_debug_overlay_only", false);
    const bool force_enable =
        GLOBALS.get_or_default<bool>("lighting_debug_force_enable", false);

    const int w = raylib::GetScreenWidth();
    const int h = raylib::GetScreenHeight();

    // Overlay-only mode: fully cover the world so we can verify order/draw path.
    if (overlay_only) {
        raylib::BeginBlendMode(raylib::BLEND_ALPHA);
        raylib::DrawRectangle(0, 0, w, h, Color{0, 0, 0, 255});
        raylib::EndBlendMode();
    }

    const bool is_night = SystemManager::get().is_bar_open();
    const bool should_apply = force_enable || is_night;

    // Phase 1: global ambience (night vs day)
    if (should_apply) {
        // Night: darken overall
        raylib::BeginBlendMode(raylib::BLEND_ALPHA);
        raylib::DrawRectangle(0, 0, w, h,
                              Color{0, 0, 0, PHASE1.night_outdoor_dark_alpha});
        raylib::EndBlendMode();

        // Night: lift indoors so it stays readable
        raylib::BeginBlendMode(raylib::BLEND_ADDITIVE);
        const auto cam = game_cam.camera;
        const float y = PHASE1.mask_y;

        const auto draw_roof_fill = [&](const Building& b) {
            const float x0 = b.area.x;
            const float z0 = b.area.y;
            const float x1 = b.area.x + b.area.width;
            const float z1 = b.area.y + b.area.height;
            const vec2 p0 = world_to_screen({x0, y, z0}, cam);
            const vec2 p1 = world_to_screen({x1, y, z0}, cam);
            const vec2 p2 = world_to_screen({x1, y, z1}, cam);
            const vec2 p3 = world_to_screen({x0, y, z1}, cam);
            draw_screen_quad(p0, p1, p2, p3, PHASE1.night_indoor_lift);
        };

        draw_roof_fill(LOBBY_BUILDING);
        draw_roof_fill(MODEL_TEST_BUILDING);
        draw_roof_fill(PROGRESSION_BUILDING);
        draw_roof_fill(STORE_BUILDING);
        draw_roof_fill(BAR_BUILDING);
        draw_roof_fill(LOAD_SAVE_BUILDING);

        raylib::EndBlendMode();
    } else {
        // Day: optional very subtle warmth (can be disabled via alpha=0)
        if (PHASE1.day_tint.a > 0) {
            raylib::BeginBlendMode(raylib::BLEND_ALPHA);
            raylib::DrawRectangle(0, 0, w, h, PHASE1.day_tint);
            raylib::EndBlendMode();
        }
    }

    // Debug label (always on when enabled)
    raylib::DrawText(
        fmt::format(
            "LIGHTING (Phase 1) [H]enable [J]overlay-only [K]force | night={} "
            "apply={} overlay_only={}",
            is_night, should_apply, overlay_only)
            .c_str(),
        20, 20, 18, WHITE);

    // Project building rectangles so we can validate world->screen projection.
    const float y = PHASE1.mask_y;
    const auto cam = game_cam.camera;
    draw_projected_building_rect_outline(LOBBY_BUILDING, cam, y, GREEN);
    draw_projected_building_rect_outline(MODEL_TEST_BUILDING, cam, y, GREEN);
    draw_projected_building_rect_outline(PROGRESSION_BUILDING, cam, y, GREEN);
    draw_projected_building_rect_outline(STORE_BUILDING, cam, y, GREEN);
    draw_projected_building_rect_outline(BAR_BUILDING, cam, y, GREEN);
    draw_projected_building_rect_outline(LOAD_SAVE_BUILDING, cam, y, GREEN);
}

}  // namespace

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

    auto act = GLOBALS.get_ptr<Entity>(strings::globals::CAM_TARGET);
    if (!act) {
        return;
    }

    cam->updateToTarget(act->get<Transform>().pos());
    cam->updateCamera();

    //         jun 24-23 we need this so furniture shows up
    auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
    if (map_ptr) {
        // NOTE: today we need to grab things so that the client renders
        // what they server has access to
        map_ptr->grab_things();
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
    auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
    const auto network_debug_mode_on =
        GLOBALS.get_or_default<bool>("network_ui_enabled", false);
    if (network_debug_mode_on) {
        map_ptr = GLOBALS.get_ptr<Map>("server_map");
    }

    raylib::BeginMode3D((*cam).get());
    {
        raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f}, (vec2){256.0f, 256.0f},
                          DARKGRAY);
        if (map_ptr) map_ptr->onDraw(dt);

        if (true || GLOBALS.get<bool>("debug_ui_enabled")) {
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

    auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);

    ext::clear_background(Color{200, 200, 200, 255});
    draw_world(dt);
    // Phase 1: baseline ambience + indoor mask (still debug-toggled)
    draw_phase1_lighting_overlay(*cam);

    // note: for ui stuff
    if (map_ptr) map_ptr->onDrawUI(dt);
}
