#pragma once

// X-macro list for PSCFG-persisted settings.
//
// Goal: make the schema look like a "regular file" next to settings::Data,
// while still generating:
// - settings::Data field declarations (with defaults)
// - settings_schema::KeySpec table entries (type + lifecycle + mapping)
//
// Each entry is:
//   X_<TYPE>(section, key, member_name, default_value, lifecycle_expr, comment)
//
// The `comment` parameter is emitted verbatim next to the field declaration so
// you can keep `@v...` annotations human-readable.

// Lifecycle helpers (removed is exclusive, so "v1-v3" => removed_in_version = 4).
#define PSCFG_V_SINCE(v) settings_schema::Lifecycle{.since = (v)}
#define PSCFG_V_RANGE_INCLUSIVE(v_from, v_to) \
    settings_schema::Lifecycle{.since = (v_from), .removed_in_version = ((v_to) + 1)}
#define PSCFG_V_DEPRECATED_SINCE(v_from, v_deprecated)           \
    settings_schema::Lifecycle {                                 \
        .since = (v_from), .deprecated_since = (v_deprecated)    \
    }
#define PSCFG_V_DEPRECATED_RANGE_INCLUSIVE(v_from, v_deprecated, v_to) \
    settings_schema::Lifecycle {                                        \
        .since = (v_from),                                              \
        .deprecated_since = (v_deprecated),                             \
        .removed_in_version = ((v_to) + 1)                              \
    }

// The canonical list (order matters for persistence).
#define SETTINGS_PSCFG_LIST(X_BOOL, X_F32, X_STR, X_I32X2)                            \
    /* [audio] */                                                                    \
    X_F32("audio", "master_volume", master_volume, 0.5f, PSCFG_V_SINCE(1), /* @v1 */)\
    X_F32("audio", "music_volume", music_volume, 0.5f, PSCFG_V_SINCE(1), /* @v1 */)  \
    X_F32("audio", "sound_volume", sound_volume, 0.5f, PSCFG_V_SINCE(1), /* @v1 */)  \
                                                                                     \
    /* [video] */                                                                    \
    X_BOOL("video", "is_fullscreen", isFullscreen, false, PSCFG_V_SINCE(1), /* @v1 */)\
    X_I32X2("video", "resolution", resolution, /*default*/ 0, PSCFG_V_SINCE(1), /* @v1 */)\
    X_BOOL("video", "vsync_enabled", vsync_enabled, true, PSCFG_V_SINCE(1), /* @v1 */)\
    X_BOOL("video", "enable_postprocessing", enable_postprocessing, true, PSCFG_V_SINCE(1), /* @v1 */)\
    X_BOOL("video", "enable_lighting", enable_lighting, true, PSCFG_V_SINCE(1), /* @v1 */)\
    X_BOOL("video", "snap_camera_to_90", snapCameraTo90, false, PSCFG_V_SINCE(1), /* @v1 */)\
                                                                                     \
    /* [ui] */                                                                       \
    X_STR("ui", "lang_name", lang_name, "en_us", PSCFG_V_SINCE(1), /* @v1 */)         \
    X_STR("ui", "ui_theme", ui_theme, "", PSCFG_V_SINCE(1), /* @v1 */)               \
    X_BOOL("ui", "show_streamer_safe_box", show_streamer_safe_box, false, PSCFG_V_SINCE(1), /* @v1 */)\
                                                                                     \
    /* [network] */                                                                  \
    X_STR("network", "username", username, "", PSCFG_V_SINCE(1), /* @v1 */)          \
    X_STR("network", "last_ip_joined", last_ip_joined, "", PSCFG_V_SINCE(1), /* @v1 */)

