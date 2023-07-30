
#pragma once

#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/graphics.h"
#include "engine/singleton.h"
#include "raylib.h"
//
#include "engine/files.h"
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

        // TODO right now these arent reloadable yet but should be soon
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
    }

    // Note: Defined in .cpp to avoid LOG_LEVEL violating C++ ODR during
    // linking.
    void load_config();

    void reload_config() {
        load_config();
        load_models();

        // drinks use models, so let those load first
        load_drink_recipes();
    }

    void load_translations() {
        // TODO load correct language pack for settings file
        auto path = Files::get().fetch_resource_path(
            strings::settings::TRANSLATIONS, "en_rev.mo");
        reload_translations_from_file(path.c_str());
    }

    void reload_translations_from_file(const char* fn) {
        if (localization) delete localization;
        localization = new i18n::LocalizationText(fn);
    }

    void load_fonts() {
        // Font loading must happen after InitWindow
        font = load_karmina_regular();

        // NOTE if you go back to files, load fonts from install folder, instead
        // of local path
        //
        // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
    }

    void load_textures();
    void load_models();
    auto load_json_config_file(
        const char* filename,
        const std::function<void(nlohmann::json)>& processor);
    void load_settings_config();
    void load_drink_recipes();

    void load_sounds() {
        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::SOUNDS,
                                     "roblox_oof.ogg")
                .c_str(),
            strings::sounds::ROBLOX);

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
                .fetch_resource_path(strings::settings::MUSIC,
                                     "supermarket.ogg")
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
