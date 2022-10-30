#pragma once

#include "external_include.h"
//
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(MusicLibrary)
struct MusicLibrary {
    SINGLETON(MusicLibrary)

    struct MusicLibraryImpl : Library<Music> {
        virtual void load(const char* filename, const char* name) override {
            // TODO add debug mode
            std::cout << "loading texture: " << name << " from " << filename
                      << std::endl;
            this->add(name, LoadMusicStream(filename));
        }
    } impl;

    Music& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
