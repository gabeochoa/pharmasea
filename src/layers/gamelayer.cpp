
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

// These live here so lighting debug works even if DebugSettingsLayer isn't loaded.
static bool g_lighting_debug_enabled = false;
static bool g_lighting_debug_overlay_only = false;
static bool g_lighting_debug_force_enable = false;

inline void ensure_lighting_debug_globals_registered() {
    if (!GLOBALS.contains("lighting_debug_enabled")) {
        GLOBALS.set("lighting_debug_enabled", &g_lighting_debug_enabled);
    }
    if (!GLOBALS.contains("lighting_debug_overlay_only")) {
        GLOBALS.set("lighting_debug_overlay_only", &g_lighting_debug_overlay_only);
    }
    if (!GLOBALS.contains("lighting_debug_force_enable")) {
        GLOBALS.set("lighting_debug_force_enable", &g_lighting_debug_force_enable);
    }
}

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
    // Indoor tint: intentionally obvious for validation.
    // Later we can replace with a subtle "lift" once placement is correct.
    Color night_indoor_tint = Color{255, 165, 0, 140};
    // Optional: day tint (kept subtle; can be turned off by setting alpha=0).
    Color day_tint = Color{255, 240, 220, 10};
    // Projection height used for debug screen-space outlines.
    // NOTE: The world ground plane is drawn at y = -TILESIZE (see draw_world()).
    // If we project at -0.5, the mask can read like it's floating above the ground.
    // For correctness, stick to the actual ground plane.
    float mask_y = -TILESIZE;

    // World-space mask Y: draw a translucent plane slightly above the ground to
    // avoid Z-fighting with the ground plane.
    float world_mask_y = (-TILESIZE) + 0.02f;
};

static const Phase1LightingTuning PHASE1{};

inline bool phase1_enabled() {
    ensure_lighting_debug_globals_registered();
    return GLOBALS.get_or_default<bool>("lighting_debug_enabled", false);
}

inline bool phase1_overlay_only() {
    ensure_lighting_debug_globals_registered();
    return GLOBALS.get_or_default<bool>("lighting_debug_overlay_only", false);
}

inline bool phase1_force_enable() {
    ensure_lighting_debug_globals_registered();
    return GLOBALS.get_or_default<bool>("lighting_debug_force_enable", false);
}

inline bool phase1_should_apply() {
    // Night in this codebase == bar open (in-round).
    return phase1_force_enable() || SystemManager::get().is_bar_open();
}

void draw_phase1_indoor_mask_worldspace() {
    if (!phase1_enabled()) return;
    if (!phase1_should_apply()) return;

    // In overlay-only mode we intentionally hide the world, so skip world-space mask.
    if (phase1_overlay_only()) return;

    raylib::BeginBlendMode(raylib::BLEND_ALPHA);

    const auto draw_building_plane = [&](const Building& b) {
        const float cx = b.area.x + (b.area.width / 2.f);
        const float cz = b.area.y + (b.area.height / 2.f);
        // DrawPlane takes size in XZ.
        raylib::DrawPlane({cx, PHASE1.world_mask_y, cz},
                          {b.area.width, b.area.height}, PHASE1.night_indoor_tint);
    };

    draw_building_plane(LOBBY_BUILDING);
    draw_building_plane(MODEL_TEST_BUILDING);
    draw_building_plane(PROGRESSION_BUILDING);
    draw_building_plane(STORE_BUILDING);
    draw_building_plane(BAR_BUILDING);
    draw_building_plane(LOAD_SAVE_BUILDING);

    raylib::EndBlendMode();
}

void draw_phase1_lighting_overlay(const GameCam& game_cam) {
    if (!phase1_enabled()) return;

    const bool overlay_only = phase1_overlay_only();
    const bool force_enable = phase1_force_enable();

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

    // Throttled debug logging to validate projection alignment.
    // We log one building's corners at multiple Y levels (ground/base/center).
    {
        static double last_log_time = 0.0;
        const double now_s = raylib::GetTime();
        if (now_s - last_log_time > 2.0) {
            last_log_time = now_s;
            const auto& b = BAR_BUILDING;
            const float x0 = b.area.x;
            const float z0 = b.area.y;
            const auto cam = game_cam.camera;
            // Match how draw_building() draws the debug cube: [x0..x0+width], [z0..z0+height]
            const float x1 = b.area.x + b.area.width;
            const float z1 = b.area.y + b.area.height;

            const float y_ground = -TILESIZE;
            const float y_base = -TILESIZE / 2.f;
            const float y_center = 0.f;

            const vec2 g0 = world_to_screen({x0, y_ground, z0}, cam);
            const vec2 g1 = world_to_screen({x1, y_ground, z0}, cam);
            const vec2 b0s = world_to_screen({x0, y_base, z0}, cam);
            const vec2 b1s = world_to_screen({x1, y_base, z0}, cam);
            const vec2 c0 = world_to_screen({x0, y_center, z0}, cam);
            const vec2 c1 = world_to_screen({x1, y_center, z0}, cam);

            log_info(
                "lighting mask: bar rect world corners=({},{})->({},{}) "
                "cam_pos={} cam_target={} | y_ground=-1 p0={} p1={} | y_base=-0.5 p0={} p1={} | y_center=0 p0={} p1={}",
                x0, z0, x1, z1, cam.position, cam.target, g0, g1, b0s, b1s, c0,
                c1);
        }
    }

    // Phase 1: global ambience (night vs day).
    // IMPORTANT: indoor masking is drawn in-world during the 3D pass to avoid
    // the "floating rectangle in the sky" artifact from screen-space overlays.
    if (should_apply) {
        // Night: darken overall
        raylib::BeginBlendMode(raylib::BLEND_ALPHA);
        raylib::DrawRectangle(0, 0, w, h,
                              Color{0, 0, 0, PHASE1.night_outdoor_dark_alpha});
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
            "apply={} overlay_only={} mask_y={} world_mask_y={}",
            is_night, should_apply, overlay_only, PHASE1.mask_y, PHASE1.world_mask_y)
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
    // Extra alignment debug: draw BAR_BUILDING outline at 2 Y levels.
    // GREEN = active mask_y, RED = ground plane. If these differ a lot, projection is off.
    draw_projected_building_rect_outline(BAR_BUILDING, cam, -TILESIZE, RED);
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

    // Phase 0/1: lighting debug toggles live here so they work even if the
    // debug settings layer isn't loaded.
    ensure_lighting_debug_globals_registered();
    if (KeyMap::get_key_code(menu::State::Game, InputName::ToggleLightingDebug) ==
        event.keycode) {
        bool v = GLOBALS.get_or_default<bool>("lighting_debug_enabled", false);
        GLOBALS.update("lighting_debug_enabled", !v);
        return true;
    }
    if (KeyMap::get_key_code(menu::State::Game,
                             InputName::ToggleLightingOverlayOnly) ==
        event.keycode) {
        bool v =
            GLOBALS.get_or_default<bool>("lighting_debug_overlay_only", false);
        GLOBALS.update("lighting_debug_overlay_only", !v);
        return true;
    }
    if (KeyMap::get_key_code(menu::State::Game,
                             InputName::ToggleLightingForceEnable) ==
        event.keycode) {
        bool v =
            GLOBALS.get_or_default<bool>("lighting_debug_force_enable", false);
        GLOBALS.update("lighting_debug_force_enable", !v);
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
