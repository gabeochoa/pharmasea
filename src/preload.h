
#pragma once

#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/graphics.h"
#include "engine/singleton.h"
#include "raylib.h"
//

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    raylib::Font font;

    Preload();
    ~Preload();

    raylib::Font load_karmina_regular();

    void load_translations_from_file(const char* fn);
    void load_translations();

    void load_fonts();
    void load_textures();
    void load_models();
    void load_sounds();
    void load_music();
    void load_shaders();
};
