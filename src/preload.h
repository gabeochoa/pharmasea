
#pragma once
#include <fstream>
#include <iostream>

#include "engine/font_util.h"
#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/graphics_types.h"
#include "engine/singleton.h"
#include "raylib.h"
//
#include "engine/files.h"
#include "engine/font_library.h"
#include "engine/model_library.h"
#include "engine/music_library.h"
#include "engine/shader_library.h"
#include "engine/sound_library.h"
#include "engine/texture_library.h"
#include "globals.h"
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

    Preload() {
        reload_config();

        // TODO :BE: right now these arent reloadable yet but should be soon
        // add support an move them to reload_config()

        load_translations();
        load_shaders();

        load_textures();
        if (ENABLE_SOUND) {
            ext::init_audio_device();
            load_sounds();
            load_music();
        }
        completed_preload_once = true;
    }

    ~Preload() {
        if (ENABLE_SOUND) {
            ext::close_audio_device();
        }

        TextureLibrary::get().unload_all();
        MusicLibrary::get().unload_all();
        ModelLibrary::get().unload_all();
        SoundLibrary::get().unload_all();
        ShaderLibrary::get().unload_all();
        FontLibrary::get().unload_all();
    }

    void write_keymap();
    void on_language_change(const char* lang_name, const char* fn);

    void reload_config() {
        load_map_generation_info();
        load_config();
        load_keymapping();
        load_models();

        // drinks use models, so let those load first
        load_drink_recipes();
    }

    void update_ui_theme(const std::string& theme);
    std::vector<std::string> ui_theme_options();

   private:
    // Note: Defined in .cpp to avoid LOG_LEVEL violating C++ ODR during
    // linking.
    void load_config();

    void load_map_generation_info();
    void load_keymapping();

    void load_translations();

    void _load_font_from_name(const std::string& filename,
                              const std::string& lang);
    const char* get_font_for_lang(const char* lang_name);
    void load_fonts(const nlohmann::json& data);

    void load_textures();
    void load_models();
    auto load_json_config_file(
        const char* filename,
        const std::function<void(nlohmann::json)>& processor);
    void write_json_config_file(const char* filename,
                                const nlohmann::json& data);
    void load_settings_config();
    void load_drink_recipes();
    void load_sounds();
    void load_music();
    void load_shaders();
};
