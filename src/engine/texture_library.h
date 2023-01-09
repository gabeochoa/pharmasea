
#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
    SINGLETON(TextureLibrary)

    struct TextureLibraryImpl : Library<raylib::Texture2D> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading texture: {} from {}", name, filename);
            this->add(name, raylib::LoadTexture(filename));
        }

        virtual void unload(raylib::Texture2D texture) override {
            raylib::UnloadTexture(texture);
        }
    } impl;

    [[nodiscard]] raylib::Texture2D& get(const std::string& name) {
        return impl.get(name);
    }
    [[nodiscard]] const raylib::Texture2D& get(const std::string& name) const {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
