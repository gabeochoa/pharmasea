

#pragma once

#include "is_server.h"
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
    SINGLETON(SoundLibrary)

    // TODO if we add a "sound effects" volume slider, we have to do the logic
    // from MusicLibrary

    [[nodiscard]] raylib::Sound& get(const std::string& name) {
        return impl.get(name);
    }
    [[nodiscard]] const raylib::Sound& get(const std::string& name) const {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }

    void play(const char* name) {
        if (is_server()) {
            log_warn(
                "You are playing sounds in the server, only the host will "
                "hear these (trying to play {})",
                name);
        }
        PlaySound(get(name));
    }

    void play_random_match(const std::string& prefix) {
        impl.get_random_match(prefix).map(raylib::PlaySound);
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct SoundLibraryImpl : Library<raylib::Sound> {
        virtual raylib::Sound convert_filename_to_object(
            const char*, const char* filename) override {
            return raylib::LoadSound(filename);
        }
        virtual void unload(raylib::Sound sound) override {
            raylib::UnloadSound(sound);
        }
    } impl;
};
