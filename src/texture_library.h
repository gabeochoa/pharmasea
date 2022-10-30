
#pragma once

#include "external_include.h"
//
#include "files.h"
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
    SINGLETON(TextureLibrary)

    struct TextureLibraryImpl : Library<Texture2D> {
        virtual void load(const char* filename, const char* name) override {
            // TODO add debug mode
            std::cout << "loading texture: " << name << " from " << filename
                      << std::endl;
            this->add(name, LoadTexture(filename));
        }
    } impl;

    Texture2D& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
