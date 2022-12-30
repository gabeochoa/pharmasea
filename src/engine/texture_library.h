
#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
    SINGLETON(TextureLibrary)

    struct TextureLibraryImpl : Library<Texture2D> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading texture: {} from {}", name, filename);
            this->add(name, LoadTexture(filename));
        }

        virtual void unload(Texture2D texture) override {
            UnloadTexture(texture);
        }
    } impl;

    Texture2D& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
