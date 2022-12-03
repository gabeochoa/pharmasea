
#pragma once

#include "external_include.h"
//
#include "raylib.h"
#include "singleton.h"
//
#include "model_library.h"
#include "music_library.h"
#include "resources/fonts/Karmina_Regular_256.h"
#include "shader_library.h"
#include "sound_library.h"
#include "texture_library.h"

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

    ~Preload() { CloseAudioDevice(); }

    void load_fonts() {
        // TODO - load fonts from install folder, instead of local path
        // Font loading must happen after InitWindow
        font = LoadFont_KarminaRegular256();
        // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    }

    void load_textures() {
        std::tuple<const char*, const char*, const char*> textures[] = {
            {"images", "face.png", "face"},
            {"images", "jug.png", "jug"},
            {"images", "sleepyico.png", "sleepy"},
        };

        for (auto& t : textures) {
            TextureLibrary::get().load(
                Files::get()
                    .fetch_resource_path(std::get<0>(t), std::get<1>(t))
                    .c_str(),
                std::get<2>(t));
        }
    }

    void load_models() {
        std::tuple<const char*, const char*, const char*> models[] = {
            {"models", "bag.obj", "bag"},
            {"models", "empty_bag.obj", "empty_bag"},
            {"models", "conveyer.obj", "conveyer"},
            {"models", "register.obj", "register"},
        };

        for (auto& m : models) {
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
        std::tuple<const char*, const char*, const char*> models[] = {
            {"shaders", "pixelated.fs", "pixelated"},
        };

        for (auto& m : models) {
            ShaderLibrary::get().load(
                Files::get()
                    .fetch_resource_path(std::get<0>(m), std::get<1>(m))
                    .c_str(),
                std::get<2>(m));
        }
    }
};
