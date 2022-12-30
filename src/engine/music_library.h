#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(MusicLibrary)
struct MusicLibrary {
    SINGLETON(MusicLibrary)

    struct MusicLibraryImpl : Library<Music> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading music: {} from {}", name, filename);
            this->add(name, LoadMusicStream(filename));
        }

        void update_volume(float new_v) {
            for (auto kv : storage) {
                SetMusicVolume(kv.second, new_v);
            }
        }

        virtual void unload(Music music) override { UnloadMusicStream(music); }
    } impl;

    Music& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }

    void update_volume(float new_v) { impl.update_volume(new_v); }
};
