
#include "gamelayer.h"

#include "../building_locations.h"
#include "../drawing_util.h"
#include "../engine/ui/color.h"
#include "../external_include.h"
#include "../globals.h"
#include "../camera.h"
#include "../engine.h"
#include "../engine/layer.h"
#include "../map.h"
#include "../system/system_manager.h"
#include "raylib.h"

namespace {

struct LightingTuning {
    // Night ambience
    float night_outdoor_ambient = 0.35f;  // multiplied over entire scene
    // Indoor "lift" applied additively inside roof rectangles
    Color night_indoor_lift = Color{40, 35, 25, 140};

    // Bar spill light
    float bar_cone_length = 10.0f;
    float bar_cone_angle_deg = 85.0f;
    int bar_cone_segments = 18;
    Color bar_cone_color = Color{255, 190, 120, 90};  // warm, additive

    // Ground Y to project from (most gameplay uses TILESIZE/-2)
    float ground_y = -TILESIZE / 2.f;
};

static const LightingTuning LIGHTING{};

inline vec2 project_to_screen(const vec3& world, const raylib::Camera3D& cam) {
    return raylib::GetWorldToScreen(world, cam);
}

inline void draw_screen_quad(const vec2& a, const vec2& b, const vec2& c,
                             const vec2& d, Color col) {
    raylib::DrawTriangle(a, b, c, col);
    raylib::DrawTriangle(a, c, d, col);
}

inline void draw_building_roof_quad(const Building& b,
                                    const raylib::Camera3D& cam, float y,
                                    Color col) {
    const float x0 = b.area.x;
    const float z0 = b.area.y;
    const float x1 = b.area.x + b.area.width;
    const float z1 = b.area.y + b.area.height;

    const vec2 p0 = project_to_screen({x0, y, z0}, cam);
    const vec2 p1 = project_to_screen({x1, y, z0}, cam);
    const vec2 p2 = project_to_screen({x1, y, z1}, cam);
    const vec2 p3 = project_to_screen({x0, y, z1}, cam);

    draw_screen_quad(p0, p1, p2, p3, col);
}

inline vec2 normalize2(vec2 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.00001f) return {0.f, 1.f};
    return {v.x / len, v.y / len};
}

inline float deg2rad(float deg) { return deg * DEG2RAD; }

void draw_bar_spill_cone(const raylib::Camera3D& cam) {
    if (BAR_BUILDING.doors.empty()) return;

    // Pick the "center" door tile from add_door() (the first push is center).
    const vec2 door = BAR_BUILDING.doors[0];
    const vec2 outside = BAR_BUILDING.vomit_location;

    vec2 dir = normalize2(outside - door);
    vec2 origin2 = door + (dir * 0.75f);

    const float base_angle = std::atan2(dir.y, dir.x);
    const float half = deg2rad(LIGHTING.bar_cone_angle_deg) * 0.5f;

    const int segs = std::max(4, LIGHTING.bar_cone_segments);

    // Soft falloff via a few layered cones (cheap “blur”).
    struct Layer {
        float length_mul;
        unsigned char alpha;
    };
    const Layer layers[] = {
        {0.85f, static_cast<unsigned char>(LIGHTING.bar_cone_color.a)},
        {1.00f, static_cast<unsigned char>(LIGHTING.bar_cone_color.a * 0.65f)},
        {1.15f, static_cast<unsigned char>(LIGHTING.bar_cone_color.a * 0.40f)},
    };

    const vec2 origin_screen =
        project_to_screen({origin2.x, LIGHTING.ground_y, origin2.y}, cam);

    for (const Layer& layer : layers) {
        Color col = LIGHTING.bar_cone_color;
        col.a = layer.alpha;

        const float length = LIGHTING.bar_cone_length * layer.length_mul;

        // Triangle fan around the arc edge.
        for (int i = 0; i < segs; i++) {
            const float t0 = (float) i / (float) segs;
            const float t1 = (float) (i + 1) / (float) segs;

            const float a0 = base_angle - half + (2.f * half) * t0;
            const float a1 = base_angle - half + (2.f * half) * t1;

            const vec2 p0w =
                origin2 + vec2{std::cos(a0), std::sin(a0)} * length;
            const vec2 p1w =
                origin2 + vec2{std::cos(a1), std::sin(a1)} * length;

            const vec2 p0s =
                project_to_screen({p0w.x, LIGHTING.ground_y, p0w.y}, cam);
            const vec2 p1s =
                project_to_screen({p1w.x, LIGHTING.ground_y, p1w.y}, cam);

            raylib::DrawTriangle(origin_screen, p0s, p1s, col);
        }
    }
}

void draw_lighting_overlay(const GameCam& game_cam) {
    // TEMP DEBUG: force-enable overlay so it's obvious it runs.
    // Once verified in-game, we should switch back to night-only.
    const bool force_enable = true;

    // Night in this codebase == bar open (in-round).
    const bool is_night = SystemManager::get().is_bar_open();
    if (!force_enable && !is_night) return;

    const auto cam = game_cam.camera;

    const int w = raylib::GetScreenWidth();
    const int h = raylib::GetScreenHeight();

    // 1) Ambient darken (outdoors baseline)
    {
        // Use a plain alpha tint for now (more obvious / more compatible).
        // At night: strong darken. At day (debug): slight darken.
        const unsigned char alpha = is_night ? 190 : 60;
        raylib::BeginBlendMode(raylib::BLEND_ALPHA);
        raylib::DrawRectangle(0, 0, w, h, Color{0, 0, 0, alpha});
        raylib::EndBlendMode();
    }

    // 2) Indoor lift (roofed rectangles are treated as “indoors/covered”)
    {
        raylib::BeginBlendMode(raylib::BLEND_ADDITIVE);
        draw_building_roof_quad(BAR_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        draw_building_roof_quad(STORE_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        draw_building_roof_quad(PROGRESSION_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        draw_building_roof_quad(LOBBY_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        draw_building_roof_quad(MODEL_TEST_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        draw_building_roof_quad(LOAD_SAVE_BUILDING, cam, LIGHTING.ground_y,
                                LIGHTING.night_indoor_lift);
        raylib::EndBlendMode();
    }

    // 3) Bar spill light (warm additive cone)
    {
        raylib::BeginBlendMode(raylib::BLEND_ADDITIVE);
        draw_bar_spill_cone(cam);
        raylib::EndBlendMode();
    }

    // Debug label (top-left) so we know this ran.
    raylib::DrawText("LIGHTING OVERLAY ACTIVE (debug)", 20, 20, 20, RED);
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
    draw_lighting_overlay(*cam);

    // note: for ui stuff
    if (map_ptr) map_ptr->onDrawUI(dt);
}
