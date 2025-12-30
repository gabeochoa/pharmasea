#include "lighting_runtime.h"

#include "building_locations.h"
#include "components/transform.h"
#include "engine/util.h"
#include "entity_query.h"
#include "entity_type.h"
#include "system/system_manager.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace {

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
    // Intensity controls (to keep shader lighting subtle)
    float sun_diffuse_intensity = 0.45f;
    float sun_spec_intensity = 0.05f;
    float point_diffuse_intensity = 0.65f;
};

static const Phase1LightingTuning PHASE1{};

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
    int sunDiffuseIntensity = -1;
    int sunSpecIntensity = -1;
    int pointDiffuseIntensity = -1;
    int roofRectCount = -1;
    int roofRects = -1;

    int pointLightCount = -1;
    int lightsPerBuilding = -1;
    int lightRectCount = -1;
    int lightRects = -1;
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
    u.sunDiffuseIntensity = raylib::GetShaderLocation(s, "sunDiffuseIntensity");
    u.sunSpecIntensity = raylib::GetShaderLocation(s, "sunSpecIntensity");
    u.pointDiffuseIntensity =
        raylib::GetShaderLocation(s, "pointDiffuseIntensity");
    u.roofRectCount = raylib::GetShaderLocation(s, "roofRectCount");
    u.roofRects = raylib::GetShaderLocation(s, "roofRects");
    u.pointLightCount = raylib::GetShaderLocation(s, "pointLightCount");
    u.lightsPerBuilding = raylib::GetShaderLocation(s, "lightsPerBuilding");
    u.lightRectCount = raylib::GetShaderLocation(s, "lightRectCount");
    u.lightRects = raylib::GetShaderLocation(s, "lightRects");
    u.pointLightsPosRadius = raylib::GetShaderLocation(s, "pointLightsPosRadius");
    u.pointLightsColor = raylib::GetShaderLocation(s, "pointLightsColor");
    return u;
}

struct IndoorLightLayout {
    static constexpr int kBuildings = 6;
    static constexpr int kLightsPerBuilding = 10;
    static constexpr int kTotalLights = kBuildings * kLightsPerBuilding;

    bool initialized = false;
    // Light rects used for indoor lights (can be tighter than roofRects)
    std::array<vec4, kBuildings> light_rects{};
    std::array<vec4, kTotalLights> lights_pos_radius{};
    std::array<vec3, kTotalLights> lights_color{};
};

static IndoorLightLayout g_indoor_layout{};

inline vec4 building_rect_minmax(const Building& b) {
    // minX, minZ, maxX, maxZ
    return {b.min().x, b.min().y, b.max().x, b.max().y};
}

inline vec4 compute_bar_light_rect_from_walls() {
    // Bar building can be larger than actual placed walls.
    // Find wall tiles inside BAR_BUILDING bounds and compute a tight interior rect.
    const auto walls = EntityQuery()
                           .whereType(EntityType::Wall)
                           .whereInside(BAR_BUILDING.min(), BAR_BUILDING.max())
                           .gen();

    if (walls.empty()) {
        return building_rect_minmax(BAR_BUILDING);
    }

    float minx = 1e9f, minz = 1e9f, maxx = -1e9f, maxz = -1e9f;
    for (const Entity& w : walls) {
        if (w.is_missing<Transform>()) continue;
        vec2 p = w.get<Transform>().as2();
        minx = fmin(minx, p.x);
        minz = fmin(minz, p.y);
        maxx = fmax(maxx, p.x);
        maxz = fmax(maxz, p.y);
    }

    // Move inward by 1 tile from walls to approximate interior.
    vec4 r = {minx + 1.f, minz + 1.f, maxx - 1.f, maxz - 1.f};

    // Validate: if invalid (door gaps or weirdness), fall back.
    if (r.z <= r.x || r.w <= r.y) {
        return building_rect_minmax(BAR_BUILDING);
    }
    return r;
}

inline void rebuild_indoor_light_layout() {
    // Build rects (roof rectangles for light placement). Only bar needs special-case today.
    g_indoor_layout.light_rects[0] = building_rect_minmax(LOBBY_BUILDING);
    g_indoor_layout.light_rects[1] = building_rect_minmax(MODEL_TEST_BUILDING);
    g_indoor_layout.light_rects[2] = building_rect_minmax(PROGRESSION_BUILDING);
    g_indoor_layout.light_rects[3] = building_rect_minmax(STORE_BUILDING);
    g_indoor_layout.light_rects[4] = compute_bar_light_rect_from_walls();
    g_indoor_layout.light_rects[5] = building_rect_minmax(LOAD_SAVE_BUILDING);

    const auto radius_for_rect = [](const vec4& r) -> float {
        float w = r.z - r.x;
        float h = r.w - r.y;
        float min_dim = fmin(w, h);
        return fmax(6.0f, 0.45f * min_dim);
    };

    const auto fill_building = [&](int building_index, const vec4& rect,
                                   const vec3& color) {
        const float minx = rect.x;
        const float minz = rect.y;
        const float maxx = rect.z;
        const float maxz = rect.w;

        // 5 x positions, 2 z positions (10 lights per building).
        const float xs[5] = {0.12f, 0.30f, 0.50f, 0.70f, 0.88f};
        const float zs[2] = {0.33f, 0.66f};
        const float ly = 4.0f;
        const float r = radius_for_rect(rect);

        // Add deterministic jitter per light to make positions less "grid obvious".
        auto jitter = [](int seed) -> float {
            // Simple hash -> [-0.35, 0.35]
            uint32_t x = (uint32_t) (seed * 2654435761u);
            x ^= x >> 16;
            float t = (x & 1023u) / 1023.0f;
            return (t - 0.5f) * 0.7f;
        };

        int base = building_index * IndoorLightLayout::kLightsPerBuilding;
        int k = 0;
        for (int zi = 0; zi < 2; zi++) {
            for (int xi = 0; xi < 5; xi++) {
                const int idx = base + k;
                const float jx = jitter(idx * 2 + 1);
                const float jz = jitter(idx * 2 + 2);
                float x = util::lerp(minx, maxx, xs[xi]) + (0.35f * jx);
                float z = util::lerp(minz, maxz, zs[zi]) + (0.35f * jz);
                // Clamp inside the rect
                x = util::clamp(x, minx + 0.25f, maxx - 0.25f);
                z = util::clamp(z, minz + 0.25f, maxz - 0.25f);
                g_indoor_layout.lights_pos_radius[idx] = vec4{x, ly, z, r};
                // Base indoor lights: keep intensity modest (no runtime boost).
                // If you want brighter/dimmer, tune this scalar.
                constexpr float indoor_intensity = 0.9f;
                g_indoor_layout.lights_color[idx] =
                    vec3{color.x * indoor_intensity, color.y * indoor_intensity,
                         color.z * indoor_intensity};
                k++;
            }
        }
    };

    // Colors per building (can be unified later).
    fill_building(0, g_indoor_layout.light_rects[0], vec3{0.95f, 0.90f, 1.00f});
    fill_building(1, g_indoor_layout.light_rects[1], vec3{1.00f, 0.85f, 0.65f});
    fill_building(2, g_indoor_layout.light_rects[2], vec3{0.80f, 1.00f, 0.85f});
    fill_building(3, g_indoor_layout.light_rects[3], vec3{0.85f, 0.92f, 1.00f});
    fill_building(4, g_indoor_layout.light_rects[4], vec3{1.00f, 0.78f, 0.55f});
    fill_building(5, g_indoor_layout.light_rects[5], vec3{0.95f, 0.85f, 0.70f});

    g_indoor_layout.initialized = true;
}

}  // namespace

void update_lighting_shader(raylib::Shader& shader, const raylib::Camera3D& cam) {
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
    set_float(shader, u.sunDiffuseIntensity, PHASE1.sun_diffuse_intensity);
    set_float(shader, u.sunSpecIntensity, PHASE1.sun_spec_intensity);
    set_float(shader, u.pointDiffuseIntensity, PHASE1.point_diffuse_intensity);

    // Roof rectangles: disable direct sun indoors.
    // vec4(minX, minZ, maxX, maxZ)
    const vec4 rects[] = {
        {LOBBY_BUILDING.min().x, LOBBY_BUILDING.min().y, LOBBY_BUILDING.max().x,
         LOBBY_BUILDING.max().y},
        {MODEL_TEST_BUILDING.min().x, MODEL_TEST_BUILDING.min().y,
         MODEL_TEST_BUILDING.max().x, MODEL_TEST_BUILDING.max().y},
        {PROGRESSION_BUILDING.min().x, PROGRESSION_BUILDING.min().y,
         PROGRESSION_BUILDING.max().x, PROGRESSION_BUILDING.max().y},
        {STORE_BUILDING.min().x, STORE_BUILDING.min().y, STORE_BUILDING.max().x,
         STORE_BUILDING.max().y},
        {BAR_BUILDING.min().x, BAR_BUILDING.min().y, BAR_BUILDING.max().x,
         BAR_BUILDING.max().y},
        {LOAD_SAVE_BUILDING.min().x, LOAD_SAVE_BUILDING.min().y,
         LOAD_SAVE_BUILDING.max().x, LOAD_SAVE_BUILDING.max().y},
    };
    const int count = 6;
    set_int(shader, u.roofRectCount, count);
    raylib::SetShaderValueV(shader, u.roofRects, rects,
                            raylib::SHADER_UNIFORM_VEC4, count);

    // Indoor point lights (always on, day + night).
    if (!g_indoor_layout.initialized) {
        rebuild_indoor_light_layout();
    }

    set_int(shader, u.lightsPerBuilding, IndoorLightLayout::kLightsPerBuilding);
    set_int(shader, u.lightRectCount, IndoorLightLayout::kBuildings);
    raylib::SetShaderValueV(shader, u.lightRects, g_indoor_layout.light_rects.data(),
                            raylib::SHADER_UNIFORM_VEC4,
                            IndoorLightLayout::kBuildings);

    set_int(shader, u.pointLightCount, IndoorLightLayout::kTotalLights);
    raylib::SetShaderValueV(shader, u.pointLightsPosRadius,
                            g_indoor_layout.lights_pos_radius.data(),
                            raylib::SHADER_UNIFORM_VEC4,
                            IndoorLightLayout::kTotalLights);

    // Lighting behavior tuning:
    // At night, only BAR_BUILDING lights remain on; other buildings are closed.
    constexpr int bar_building_index = 4;

    std::array<vec3, IndoorLightLayout::kTotalLights> colors =
        g_indoor_layout.lights_color;
    for (int b = 0; b < IndoorLightLayout::kBuildings; b++) {
        const int base = b * IndoorLightLayout::kLightsPerBuilding;
        const bool enabled = (!is_night) || (b == bar_building_index);

        for (int i = 0; i < IndoorLightLayout::kLightsPerBuilding; i++) {
            const int idx = base + i;
            if (!enabled) {
                colors[idx] = vec3{0.0f, 0.0f, 0.0f};
            }
        }
    }

    raylib::SetShaderValueV(shader, u.pointLightsColor, colors.data(),
                            raylib::SHADER_UNIFORM_VEC3,
                            IndoorLightLayout::kTotalLights);
}

