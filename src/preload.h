
#pragma once

#include "external_include.h"
//
#include "raylib.h"
#include "singleton.h"
//
#include "modellibrary.h"
#include "music_library.h"
#include "resources/fonts/Karmina_Regular_256.h"
#include "sound_library.h"
#include "texture_library.h"

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    raylib::Font font;

    Preload() {
        load_fonts();
        load_textures();
        load_models();
        //
        raylib::InitAudioDevice();
        load_sounds();
        load_music();
    }

    ~Preload() { raylib::CloseAudioDevice(); }

    void load_fonts() {
        // TODO - load fonts from install folder, instead of local path
        // Font loading must happen after InitWindow
        font = raylib::LoadFont_KarminaRegular256();
        // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
        GenTextureMipmaps(&font.texture);
        raylib::SetTextureFilter(font.texture, raylib::TEXTURE_FILTER_POINT);
    }
    void load_textures() {
        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "face.png").c_str(),
            "face");

        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "jug.png").c_str(),
            "jug");

        TextureLibrary::get().load(
            Files::get().fetch_resource_path("images", "sleepyico.png").c_str(),
            "bubble");
    }
    void load_models() {
        ModelLibrary::get().load(
            Files::get().fetch_resource_path("models", "bag.obj").c_str(),
            "bag");

        ModelLibrary::get().load(
            Files::get().fetch_resource_path("models", "register.obj").c_str(),
            "register");
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
};
