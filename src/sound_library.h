

#pragma once

#include "external_include.h"
//
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
    SINGLETON(SoundLibrary)

    struct SoundLibraryImpl : Library<Sound> {
        virtual void load(const char* filename, const char* name) override {
            // TODO add debug mode
            std::cout << "loading texture: " << name << " from " << filename
                      << std::endl;
            this->add(name, LoadSound(filename));
        }
        virtual void unload(Sound sound) override { UnloadSound(sound); }
    } impl;

    Sound& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
