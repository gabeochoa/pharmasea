
#pragma once

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

    raylib::Font font;

    Preload() {
        load_shaders();

        load_fonts();
        load_textures();
        load_models();
        //
        ext::init_audio_device();
        load_sounds();
        load_music();
    }

    ~Preload() {
        ext::close_audio_device();

        // TODO instead of leaving the impl public fix these to do what music
        // library does
        TextureLibrary::get().impl.unload_all();
        ModelLibrary::get().impl.unload_all();
        SoundLibrary::get().impl.unload_all();
        MusicLibrary::get().unload_all();
        ShaderLibrary::get().impl.unload_all();
    }

    void load_fonts() {
        // Font loading must happen after InitWindow
        font = load_karmina_regular();

        // NOTE if you go back to files, load fonts from install folder, instead
        // of local path
        //
        // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
    }

    void load_textures() {
        const std::tuple<const char*, const char*, const char*> textures[] = {
            {"images", "face.png", "face"},
            {"images", "jug.png", "jug"},
            {"images", "sleepyico.png", "sleepy"},
        };

        for (const auto& t : textures) {
            TextureLibrary::get().load(
                Files::get()
                    .fetch_resource_path(std::get<0>(t), std::get<1>(t))
                    .c_str(),
                std::get<2>(t));
        }
    }

    void load_models() {
        const std::tuple<const char*, const char*, const char*> models[] = {
            {"models", "bag.obj", "bag"},
            {"models", "empty_bag.obj", "empty_bag"},
            {"models", "conveyer.obj", "conveyer"},
            {"models", "register.obj", "register"},
            {"models/kennynl", "snow_detail_crystal.obj", "pill_bottle"},
            {"models/kennynl", "boxOpen.obj", "open_box"},
            {"models/kennynl", "box.obj", "box"},
        };

        for (const auto& m : models) {
            ModelLibrary::get().load(
                Files::get()
                    .fetch_resource_path(std::get<0>(m), std::get<1>(m))
                    .c_str(),
                std::get<2>(m));
        }
    }

    void load_sounds() {
        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path("sounds", "roblox_oof.ogg")
                .c_str(),
            "roblox");

        Files::get().for_resources_in_folder(
            "sounds", "pa_announcements",
            [](const std::string& name, const std::string& filename) {
                SoundLibrary::get().load(
                    filename.c_str(),
                    fmt::format("pa_announcements_{}", name).c_str());
            });
    }

    void load_music() {
        MusicLibrary::get().load(
            Files::get()
                .fetch_resource_path("music", "supermarket.ogg")
                .c_str(),
            "supermarket");

        MusicLibrary::get().load(
            Files::get().fetch_resource_path("music", "theme.ogg").c_str(),
            "theme");
    }

    void load_shaders() {
        std::tuple<const char*, const char*, const char*> shaders[] = {
            {"shaders", "post_processing.fs", "post_processing"},
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
