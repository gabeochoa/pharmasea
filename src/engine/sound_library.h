

#pragma once

#include "../ah.h"
#include "is_server.h"
#include "singleton.h"

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
    SINGLETON(SoundLibrary)

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
                "hear these (trying to play '{}')",
                name);
        }
        PlaySound(get(name));
    }

    void play_random_match(const std::string& prefix) {
        impl.get_random_match(prefix).map(raylib::PlaySound);
    }

    void update_volume(float new_v) {
        impl.update_volume(new_v);
        current_volume = new_v;
    }

    void unload_all() { impl.unload_all(); }

   private:
    // Note: Read note in MusicLibrary
    float current_volume = 1.f;

    struct SoundLibraryImpl : afterhours::Library<raylib::Sound> {
        virtual raylib::Sound convert_filename_to_object(
            const char*, const char* filename) override {
            return raylib::LoadSound(filename);
        }
        virtual void unload(raylib::Sound sound) override {
            raylib::UnloadSound(sound);
        }

        void update_volume(float new_v) {
            for (const auto& kv : storage) {
                log_trace("updating sound volume for {} to {}", kv.first,
                          new_v);
                raylib::SetSoundVolume(kv.second, new_v);
            }
        }
    } impl;
};
