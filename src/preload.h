
#pragma once
#include <fstream>
#include <iostream>

#include "engine/font_util.h"
#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/graphics.h"
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
#include "strings.h"

inline raylib::Font load_karmina_regular() {
    auto font = font::LoadFont_KarminaRegular256();
    raylib::GenTextureMipmaps(&font.texture);
    raylib::SetTextureFilter(font.texture, raylib::TEXTURE_FILTER_POINT);
    return font;
}

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    raylib::Font font;

    Preload() {
        reload_config();

        // TODO :BE: right now these arent reloadable yet but should be soon
        // add support an move them to reload_config()

        load_translations();
        load_shaders();

        load_fonts();
        load_textures();
        if (ENABLE_SOUND) {
            ext::init_audio_device();
            load_sounds();
            load_music();
        }
    }

    ~Preload() {
        delete localization;

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

    // Note: Defined in .cpp to avoid LOG_LEVEL violating C++ ODR during
    // linking.
    void load_config();

    void reload_config() {
        load_map_generation_info();
        load_config();
        load_keymapping();
        load_models();

        // drinks use models, so let those load first
        load_drink_recipes();
    }

    void load_map_generation_info();
    void load_keymapping();

    void load_translations() {
        // TODO :IMPACT: load correct language pack for settings file
        auto path = Files::get().fetch_resource_path(
            strings::settings::TRANSLATIONS, "en_us.mo");
        on_language_change("en_us", path.c_str());
    }

    void update_ui_theme(const std::string& theme);
    std::vector<std::string> ui_theme_options();

    void on_language_change(const char* lang_name, const char* fn);
    void _load_font_from_name(const std::string& filename,
                              const std::string& lang);
    const char* get_font_for_lang(const char* lang_name);
    void load_fonts();
    void load_textures();
    void load_models();
    auto load_json_config_file(
        const char* filename,
        const std::function<void(nlohmann::json)>& processor);
    void write_json_config_file(const char* filename,
                                const nlohmann::json& data);
    void write_keymap();
    void load_settings_config();
    void load_drink_recipes();

    void load_sounds() {
        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::SOUNDS,
                                     "roblox_oof.ogg")
                .c_str(),
            strings::sounds::ROBLOX);

        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::SOUNDS, "vom.wav")
                .c_str(),
            strings::sounds::VOMIT);

        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::SOUNDS, "select.ogg")
                .c_str(),
            strings::sounds::SELECT);

        SoundLibrary::get().load(
            Files::get()
                // TODO replace sound
                .fetch_resource_path(strings::settings::SOUNDS, "select.ogg")
                .c_str(),
            strings::sounds::CLICK);

        Files::get().for_resources_in_folder(
            strings::settings::SOUNDS, "pa_announcements",
            [](const std::string& name, const std::string& filename) {
                SoundLibrary::get().load(
                    filename.c_str(),
                    fmt::format("pa_announcements_{}", name).c_str());
            });
    }

    void load_music() {
        MusicLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::MUSIC, "jaunt.ogg")
                .c_str(),
            "supermarket");

        MusicLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::MUSIC, "theme.ogg")
                .c_str(),
            "theme");
    }

    void load_shaders() {
        const std::tuple<const char*, const char*, const char*> shaders[] = {
            {strings::settings::SHADERS, "post_processing.fs",
             "post_processing"},
            {strings::settings::SHADERS, "discard_alpha.fs", "discard_alpha"},
        };

        for (const auto& s : shaders) {
            ShaderLibrary::get().load(
                Files::get()
                    .fetch_resource_path(std::get<0>(s), std::get<1>(s))
                    .c_str(),
                std::get<2>(s));
        }
    }
};
