
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

// TODO turned off to speed up launch time
#define ENABLE_SOUND 0

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

#if ENABLE_SOUND
        ext::init_audio_device();
        load_sounds();
        load_music();
#endif
    }

    ~Preload() {
        ext::close_audio_device();

        TextureLibrary::get().unload_all();
        MusicLibrary::get().unload_all();
        ModelLibrary::get().unload_all();
        SoundLibrary::get().unload_all();
        ShaderLibrary::get().unload_all();
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
        // TODO add support for model groups
        //
        constexpr ModelLibrary::ModelLoadingInfo models[] = {
            ModelLibrary::ModelLoadingInfo{
                .folder = "models",
                .filename = "bag.obj",
                .libraryname = "bag",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models",
                .filename = "empty_bag.obj",
                .libraryname = "empty_bag",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "bread.obj",
                .libraryname = "conveyer",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models",
                .filename = "register.obj",
                .libraryname = "register",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "can.obj",
                .libraryname = "pill_bottle",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "boxOpen.glb",
                .libraryname = "open_box",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "box.glb",
                .libraryname = "box",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "barrel.glb",
                .libraryname = "medicine_cabinet",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "pillow.glb",
                .libraryname = "pill_red",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "pillowBlue.glb",
                .libraryname = "pill_blue",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "pillowLong.glb",
                .libraryname = "pill_redlong",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kennynl",
                .filename = "pillowBlueLong.glb",
                .libraryname = "pill_bluelong",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kaykit",
                .filename = "character_bear.gltf",
                .libraryname = "character_bear",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kaykit",
                .filename = "character_dog.gltf",
                .libraryname = "character_dog",
            },
            ModelLibrary::ModelLoadingInfo{
                .folder = "models/kaykit",
                .filename = "character_duck.gltf",
                .libraryname = "character_duck",
            },
        };

        for (const auto& m : models) {
            ModelLibrary::get().load(m);
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
