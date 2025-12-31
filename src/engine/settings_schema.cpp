#include "settings_schema.h"

namespace settings_schema {
namespace {

static void get_resolution(const settings::Data& d, int32_t& a, int32_t& b) {
    a = (int32_t) d.resolution.width;
    b = (int32_t) d.resolution.height;
}

static void set_resolution(settings::Data& d, int32_t a, int32_t b) {
    d.resolution.width = (int) a;
    d.resolution.height = (int) b;
}

// Helper for concise lifecycle notation (all current keys are v1 for now).
static constexpr Lifecycle v1() { return Lifecycle{.since = 1}; }

}  // namespace

const std::vector<KeySpec>& all_keys() {
    // Note: keep this list stable & deterministic; it defines persistence order.
    static const std::vector<KeySpec> keys = {
        // --- audio ---
        KeySpec{.section = "audio",
                .key = "master_volume",
                .type = ValueType::F32,
                .lifecycle = v1(),
                .f32_member = &settings::Data::master_volume},
        KeySpec{.section = "audio",
                .key = "music_volume",
                .type = ValueType::F32,
                .lifecycle = v1(),
                .f32_member = &settings::Data::music_volume},
        KeySpec{.section = "audio",
                .key = "sound_volume",
                .type = ValueType::F32,
                .lifecycle = v1(),
                .f32_member = &settings::Data::sound_volume},

        // --- video ---
        KeySpec{.section = "video",
                .key = "is_fullscreen",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::isFullscreen},
        KeySpec{.section = "video",
                .key = "resolution",
                .type = ValueType::I32x2,
                .lifecycle = v1(),
                .get_i32x2 = &get_resolution,
                .set_i32x2 = &set_resolution},
        KeySpec{.section = "video",
                .key = "vsync_enabled",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::vsync_enabled},
        KeySpec{.section = "video",
                .key = "enable_postprocessing",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::enable_postprocessing},
        KeySpec{.section = "video",
                .key = "enable_lighting",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::enable_lighting},
        KeySpec{.section = "video",
                .key = "snap_camera_to_90",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::snapCameraTo90},

        // --- ui ---
        KeySpec{.section = "ui",
                .key = "lang_name",
                .type = ValueType::Str,
                .lifecycle = v1(),
                .str_member = &settings::Data::lang_name},
        KeySpec{.section = "ui",
                .key = "ui_theme",
                .type = ValueType::Str,
                .lifecycle = v1(),
                .str_member = &settings::Data::ui_theme},
        KeySpec{.section = "ui",
                .key = "show_streamer_safe_box",
                .type = ValueType::Bool,
                .lifecycle = v1(),
                .bool_member = &settings::Data::show_streamer_safe_box},

        // --- network ---
        KeySpec{.section = "network",
                .key = "username",
                .type = ValueType::Str,
                .lifecycle = v1(),
                .str_member = &settings::Data::username},
        KeySpec{.section = "network",
                .key = "last_ip_joined",
                .type = ValueType::Str,
                .lifecycle = v1(),
                .str_member = &settings::Data::last_ip_joined},
    };

    return keys;
}

const KeySpec* find_key(std::string_view key) {
    // N is tiny; linear scan is fine and keeps things simple.
    for (const auto& k : all_keys()) {
        if (k.key == key) return &k;
    }
    return nullptr;
}

}  // namespace settings_schema

