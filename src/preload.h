
#pragma once

#include "engine/globals.h"
#include "external_include.h"
//
#include "engine/files.h"
#include "engine/graphics.h"
#include "engine/log.h"
#include "engine/singleton.h"
#include "raylib.h"
#include "strings.h"
//

inline void load_translations_from_file(const char* fn) {
    if (localization) delete localization;

    log_info("Loading language file: {}", fn);
    localization = new i18n::LocalizationText();
    bool success = localization->init(fn);

    if (!success) {
        log_error("Failed to load localization file @ {}", fn);
    }
}

SINGLETON_FWD(Preload)
struct Preload {
    SINGLETON(Preload)

    raylib::Font font;

    Preload();
    ~Preload();

    raylib::Font load_karmina_regular();

    void load_translations() {
        log_info("loading translations from file constructor");
        load_translations_from_file(
            Files::get()
                .fetch_resource_path(strings::settings::TRANSLATIONS,
                                     "en_rev.mo")
                .c_str());
    }

    void load_fonts();
    void load_textures();
    void load_models();
    void load_sounds();
    void load_music();
    void load_shaders();
};
