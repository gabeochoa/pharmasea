#pragma once

#include <bit>
#include <cstdint>

#include "../engine/assert.h"
#include "../engine/settings_pscfg.h"
#include "../engine/settings_schema.h"

namespace tests {

inline void test_settings_pscfg_load_basic() {
    const settings::Data defaults{};

    const char* txt = R"pscfg(
version: 1;

[audio]
master_volume* = f32(0x3F000000); // 0.5
music_volume = f32(0x3E99999A);   // non-star => ignored
sound_volume* = f32(0x00000000);

[video]
is_fullscreen* = true;
resolution* = i32x2(1920, 1080);
vsync_enabled* = false;

[ui]
lang_name* = str("en_us");
ui_theme*  = str("pharmasea");

[network]
username* = str("bob");
last_ip_joined* = str("127.0.0.1");
)pscfg";

    const auto res =
        settings_pscfg::load_from_string(txt, defaults,
                                         settings_schema::PSCFG_VERSION);

    VALIDATE(!res.used_defaults, "expected settings file to be accepted");

    VALIDATE(res.data.isFullscreen == true, "expected fullscreen true");
    VALIDATE(res.data.vsync_enabled == false, "expected vsync false");
    VALIDATE(res.data.resolution.width == 1920, "expected width 1920");
    VALIDATE(res.data.resolution.height == 1080, "expected height 1080");
    VALIDATE(res.data.username == "bob", "expected username bob");
    VALIDATE(res.data.last_ip_joined == "127.0.0.1", "expected ip");
    VALIDATE(res.data.lang_name == "en_us", "expected lang");
    VALIDATE(res.data.ui_theme == "pharmasea", "expected theme");

    // Non-star should be ignored (keep default).
    VALIDATE(std::bit_cast<uint32_t>(res.data.music_volume) ==
                 std::bit_cast<uint32_t>(defaults.music_volume),
             "expected non-star music_volume to keep default");

    // f32(0x3F000000) == 0.5 exactly.
    VALIDATE(std::bit_cast<uint32_t>(res.data.master_volume) == 0x3F000000u,
             "expected master_volume bits to match exactly");
    VALIDATE(std::bit_cast<uint32_t>(res.data.sound_volume) == 0x00000000u,
             "expected sound_volume bits to match exactly");
}

inline void test_settings_pscfg_version_mismatch_defaults() {
    const settings::Data defaults{};
    const char* txt = R"pscfg(
version: 999;
[audio]
master_volume* = f32(0x3F000000);
)pscfg";

    const auto res =
        settings_pscfg::load_from_string(txt, defaults,
                                         settings_schema::PSCFG_VERSION);
    VALIDATE(res.used_defaults, "expected version mismatch to use defaults");
    VALIDATE(std::bit_cast<uint32_t>(res.data.master_volume) ==
                 std::bit_cast<uint32_t>(defaults.master_volume),
             "expected defaults to be preserved");
}

inline void test_settings_pscfg_write_roundtrip() {
    const settings::Data defaults{};

    settings::Data cur = defaults;
    // Set music_volume to exactly 0.3 via raw bits.
    cur.music_volume = std::bit_cast<float>(0x3E99999Au);

    const std::string out =
        settings_pscfg::write_overrides_only(cur, defaults,
                                             settings_schema::PSCFG_VERSION);
    const auto loaded =
        settings_pscfg::load_from_string(out, defaults,
                                         settings_schema::PSCFG_VERSION);

    VALIDATE(!loaded.used_defaults, "expected written file to load");
    VALIDATE(std::bit_cast<uint32_t>(loaded.data.music_volume) == 0x3E99999Au,
             "expected music_volume to round-trip by bits");
}

inline void test_settings_pscfg() {
    test_settings_pscfg_load_basic();
    test_settings_pscfg_version_mismatch_defaults();
    test_settings_pscfg_write_roundtrip();
}

}  // namespace tests

