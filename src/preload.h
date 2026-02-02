
#pragma once
#include <fstream>
#include <iostream>

#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/graphics.h"
#include "engine/singleton.h"
#include "raylib.h"
//
#include "afterhours/src/plugins/sound_system.h"
#include "engine/files.h"
#include "globals.h"
#include "libraries/font_library.h"
#include "libraries/model_library.h"
#include "libraries/shader_library.h"
#include "libraries/texture_atlas.h"
#include "libraries/texture_library.h"
#include "resources/fonts/Karmina_Regular_256.h"

inline raylib::Font load_karmina_regular() {
    auto font = font::LoadFont_KarminaRegular256();
    raylib::GenTextureMipmaps(&font.texture);
    raylib::SetTextureFilter(font.texture, raylib::TEXTURE_FILTER_POINT);
    return font;
}

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    bool completed_preload_once = false;
    raylib::Font font;

    Preload();

    ~Preload() {
        if (ENABLE_SOUND) {
            ext::close_audio_device();
        }

        TextureLibrary::get().unload_all();
        TextureAtlasLibrary::get().unload_all();
        afterhours::sound_system::MusicLibrary::get().unload_all();
        ModelLibrary::get().unload_all();
        afterhours::sound_system::SoundLibrary::get().unload_all();
        ShaderLibrary::get().unload_all();
        FontLibrary::get().unload_all();
    }

    void write_keymap();
    void on_language_change(const char* lang_name, const char* fn);

    void reload_config() {
        load_map_generation_info();
        load_config();
        load_keymapping();
        load_translations();
    }

    void update_ui_theme(const std::string& theme);
    std::vector<std::string> ui_theme_options();

   private:
    // Note: Defined in .cpp to avoid LOG_LEVEL violating C++ ODR during
    // linking.
    void load_config();

    void load_map_generation_info();
    void load_keymapping();
    void load_model_metadata_only(const std::function<void()>& tick);

    void load_translations();

    void _load_font_from_name(const std::string& filename,
                              const std::string& lang);
    const char* get_font_for_lang(const char* lang_name);
    void load_fonts(const nlohmann::json& data);

    void load_textures(const std::function<void()>& tick);
    void load_models(const std::function<void()>& tick);
    auto load_json_config_file(
        const char* filename,
        const std::function<void(nlohmann::json)>& processor);
    void write_json_config_file(const char* filename,
                                const nlohmann::json& data);
    void load_settings_config();
    void load_drink_recipes(const std::function<void()>& tick);
    void load_sounds(const std::function<void()>& tick);
    void load_music(const std::function<void()>& tick);
    void load_shaders(const std::function<void()>& tick);
};
