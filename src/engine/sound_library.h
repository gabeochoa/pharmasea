

#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
    SINGLETON(SoundLibrary)

    // TODO if we add a "sound effects" volume slider, we have to do the logic
    // from MusicLibrary

    struct SoundLibraryImpl : Library<raylib::Sound> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading sound: {} from {}", name, filename);
            this->add(name, raylib::LoadSound(filename));
        }
        virtual void unload(raylib::Sound sound) override {
            raylib::UnloadSound(sound);
        }
    } impl;

    [[nodiscard]] raylib::Sound& get(const std::string& name) {
        return impl.get(name);
    }
    [[nodiscard]] const raylib::Sound& get(const std::string& name) const {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }

    void play(const char* name) { PlaySound(get(name)); }

    void play_random_match(const std::string& prefix) {
        impl.get_random_match(prefix).map(raylib::PlaySound);
    }
};
