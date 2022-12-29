
#pragma once

#include "external_include.h"
//
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

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    Font font;

    Preload() {
        load_shaders();

        load_fonts();
        load_textures();
        load_models();
        //
        InitAudioDevice();
        load_sounds();
        load_music();
    }

    ~Preload() {
        CloseAudioDevice();

        TextureLibrary::get().impl.unload_all();
        ModelLibrary::get().impl.unload_all();
        SoundLibrary::get().impl.unload_all();
        MusicLibrary::get().impl.unload_all();
        ShaderLibrary::get().impl.unload_all();
    }

    void load_fonts() {
        // TODO - load fonts from install folder, instead of local path
        // Font loading must happen after InitWindow
        font = LoadFont_KarminaRegular256();
        // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
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
    }

    void load_music() {
        MusicLibrary::get().load(
            Files::get().fetch_resource_path("music", "wah.ogg").c_str(),
            "wah");
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
