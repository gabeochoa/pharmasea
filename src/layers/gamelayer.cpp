
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
#include "../engine/shader_library.h"
#include "raylib.h"

namespace {

// These live here so lighting debug works even if DebugSettingsLayer isn't loaded.
static bool g_lighting_debug_enabled = true;
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
    // Ambient term
    vec3 ambient = {0.20f, 0.20f, 0.22f};

    // "Sun": user requested point-light-like sun (stylized).
    // Note: physically, the sun should be directional. We can swap later.
    int light_type = 1;  // 0=directional, 1=point
    vec3 sun_dir = {-1.0f, -1.0f, -0.3f};     // used if directional
    vec3 sun_pos = {0.0f, 40.0f, 0.0f};       // used if point
    vec3 sun_color = {1.0f, 0.98f, 0.92f};    // warm-ish

    // Shading controls
    float shininess = 48.0f;
    bool use_half_lambert = true;

    // Debug outlines plane
    float debug_outline_y = -TILESIZE;
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

inline void set_vec3(raylib::Shader& s, int loc, const vec3& v) {
    raylib::SetShaderValue(s, loc, &v, raylib::SHADER_UNIFORM_VEC3);
}

inline void set_int(raylib::Shader& s, int loc, int v) {
    raylib::SetShaderValue(s, loc, &v, raylib::SHADER_UNIFORM_INT);
}

inline void set_float(raylib::Shader& s, int loc, float v) {
    raylib::SetShaderValue(s, loc, &v, raylib::SHADER_UNIFORM_FLOAT);
}

inline void set_bool(raylib::Shader& s, int loc, bool v) {
    int iv = v ? 1 : 0;
    raylib::SetShaderValue(s, loc, &iv, raylib::SHADER_UNIFORM_INT);
}

struct LightingUniforms {
    int viewPos = -1;
    int lightType = -1;
    int lightDir = -1;
    int lightPos = -1;
    int lightColor = -1;
    int ambientColor = -1;
    int shininess = -1;
    int useHalfLambert = -1;
    int roofRectCount = -1;
    int roofRects = -1;

    int pointLightCount = -1;
    int pointLightsPosRadius = -1;
    int pointLightsColor = -1;
};

inline LightingUniforms get_lighting_uniforms(raylib::Shader& s) {
    LightingUniforms u;
    u.viewPos = raylib::GetShaderLocation(s, "viewPos");
    u.lightType = raylib::GetShaderLocation(s, "lightType");
    u.lightDir = raylib::GetShaderLocation(s, "lightDir");
    u.lightPos = raylib::GetShaderLocation(s, "lightPos");
    u.lightColor = raylib::GetShaderLocation(s, "lightColor");
    u.ambientColor = raylib::GetShaderLocation(s, "ambientColor");
    u.shininess = raylib::GetShaderLocation(s, "shininess");
    u.useHalfLambert = raylib::GetShaderLocation(s, "useHalfLambert");
    u.roofRectCount = raylib::GetShaderLocation(s, "roofRectCount");
    u.roofRects = raylib::GetShaderLocation(s, "roofRects");
    u.pointLightCount = raylib::GetShaderLocation(s, "pointLightCount");
    u.pointLightsPosRadius =
        raylib::GetShaderLocation(s, "pointLightsPosRadius");
    u.pointLightsColor = raylib::GetShaderLocation(s, "pointLightsColor");
    return u;
}

inline void update_lighting_shader(raylib::Shader& shader,
                                  const raylib::Camera3D& cam) {
    static LightingUniforms u = get_lighting_uniforms(shader);

    // Camera
    set_vec3(shader, u.viewPos, cam.position);

    // Sun
    const bool is_night = SystemManager::get().is_bar_open();
    const vec3 sun_color = is_night ? vec3{0.0f, 0.0f, 0.0f} : PHASE1.sun_color;
    set_int(shader, u.lightType, PHASE1.light_type);
    set_vec3(shader, u.lightDir, PHASE1.sun_dir);
    set_vec3(shader, u.lightPos, PHASE1.sun_pos);
    set_vec3(shader, u.lightColor, sun_color);
    set_vec3(shader, u.ambientColor, PHASE1.ambient);

    set_float(shader, u.shininess, PHASE1.shininess);
    set_bool(shader, u.useHalfLambert, PHASE1.use_half_lambert);

    // Roof rectangles: disable direct sun indoors.
    // vec4(minX, minZ, maxX, maxZ)
    const vec4 rects[] = {
        {LOBBY_BUILDING.min().x, LOBBY_BUILDING.min().y, LOBBY_BUILDING.max().x, LOBBY_BUILDING.max().y},
        {MODEL_TEST_BUILDING.min().x, MODEL_TEST_BUILDING.min().y, MODEL_TEST_BUILDING.max().x, MODEL_TEST_BUILDING.max().y},
        {PROGRESSION_BUILDING.min().x, PROGRESSION_BUILDING.min().y, PROGRESSION_BUILDING.max().x, PROGRESSION_BUILDING.max().y},
        {STORE_BUILDING.min().x, STORE_BUILDING.min().y, STORE_BUILDING.max().x, STORE_BUILDING.max().y},
        {BAR_BUILDING.min().x, BAR_BUILDING.min().y, BAR_BUILDING.max().x, BAR_BUILDING.max().y},
        {LOAD_SAVE_BUILDING.min().x, LOAD_SAVE_BUILDING.min().y, LOAD_SAVE_BUILDING.max().x, LOAD_SAVE_BUILDING.max().y},
    };
    const int count = 6;
    set_int(shader, u.roofRectCount, count);
    raylib::SetShaderValueV(shader, u.roofRects, rects, raylib::SHADER_UNIFORM_VEC4, count);

    // Indoor point lights (always on, day + night).
    // 8 lights per building (4x2 grid), for clearer indoor lighting testing.
    // NOTE: This is intentionally heavier; we can optimize later.
    constexpr int lights_per_building = 8;
    constexpr int num_buildings = 6;
    constexpr int total_lights = lights_per_building * num_buildings;  // 48

    std::array<vec4, total_lights> pls{};
    std::array<vec3, total_lights> cols{};

    const auto radius_for = [](const Building& b) -> float {
        // Enough to overlap slightly but not flood huge areas.
        const float min_dim = fmin(b.area.width, b.area.height);
        return fmax(6.0f, 0.45f * min_dim);
    };

    const auto fill_building_lights = [&](int building_index, const Building& b,
                                          const vec3& color) {
        const float minx = b.area.x;
        const float minz = b.area.y;
        const float maxx = b.area.x + b.area.width;
        const float maxz = b.area.y + b.area.height;

        // 4 x positions, 2 z positions (even-ish distribution).
        const float xs[4] = {0.2f, 0.4f, 0.6f, 0.8f};
        const float zs[2] = {0.33f, 0.66f};

        const float ly = 4.0f;  // slightly above typical entity height
        const float r = radius_for(b);

        int base = building_index * lights_per_building;
        int k = 0;
        for (int zi = 0; zi < 2; zi++) {
            for (int xi = 0; xi < 4; xi++) {
                const float x = util::lerp(minx, maxx, xs[xi]);
                const float z = util::lerp(minz, maxz, zs[zi]);
                pls[base + k] = vec4{x, ly, z, r};
                cols[base + k] = color;
                k++;
            }
        }
    };

    // Slightly distinct colors per building so you can tell which lights belong where.
    fill_building_lights(0, LOBBY_BUILDING, vec3{0.95f, 0.90f, 1.00f});
    fill_building_lights(1, MODEL_TEST_BUILDING, vec3{1.00f, 0.85f, 0.65f});
    fill_building_lights(2, PROGRESSION_BUILDING, vec3{0.80f, 1.00f, 0.85f});
    fill_building_lights(3, STORE_BUILDING, vec3{0.85f, 0.92f, 1.00f});
    fill_building_lights(4, BAR_BUILDING, vec3{1.00f, 0.78f, 0.55f});
    fill_building_lights(5, LOAD_SAVE_BUILDING, vec3{0.95f, 0.85f, 0.70f});

    set_int(shader, u.pointLightCount, total_lights);
    raylib::SetShaderValueV(shader, u.pointLightsPosRadius, pls.data(),
                            raylib::SHADER_UNIFORM_VEC4, total_lights);
    raylib::SetShaderValueV(shader, u.pointLightsColor, cols.data(),
                            raylib::SHADER_UNIFORM_VEC3, total_lights);
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

    // Phase 1 is now shader-based lighting; overlay just shows debug info.
    const bool is_night = SystemManager::get().is_bar_open();
    const bool should_apply = force_enable || is_night;

    // Debug label (always on when enabled)
    raylib::DrawText(
        fmt::format(
            "LIGHTING (Phase 1 shader) [H]enable [J]overlay-only [K]force | night={} apply={} overlay_only={}",
            is_night, should_apply, overlay_only)
            .c_str(),
        20, 20, 18, WHITE);

    // Project building rectangles so we can validate world->screen projection.
    const float y = PHASE1.debug_outline_y;
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

    // Phase 1: shader-based lighting setup (Half-Lambert + Blinn-Phong).
    // Enable/disable via H (lighting_debug_enabled). J = overlay-only (hide world).
    ensure_lighting_debug_globals_registered();
    const bool lighting_enabled =
        GLOBALS.get_or_default<bool>("lighting_debug_enabled", false);
    const bool overlay_only =
        GLOBALS.get_or_default<bool>("lighting_debug_overlay_only", false);

    raylib::Shader* lighting_shader_ptr = nullptr;
    if (lighting_enabled) {
        lighting_shader_ptr = &ShaderLibrary::get().get("lighting");
    }

    raylib::BeginMode3D((*cam).get());
    {
        if (!overlay_only) {
            if (lighting_shader_ptr) {
                update_lighting_shader(*lighting_shader_ptr, (*cam).get());
                raylib::BeginShaderMode(*lighting_shader_ptr);
            }

            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){256.0f, 256.0f}, DARKGRAY);
            if (map_ptr) map_ptr->onDraw(dt);

            if (lighting_shader_ptr) {
                raylib::EndShaderMode();
            }
        }

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
