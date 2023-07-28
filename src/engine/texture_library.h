
#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
    SINGLETON(TextureLibrary)

    [[nodiscard]] auto size() { return impl.size(); }
    void unload_all() { impl.unload_all(); }

    [[nodiscard]] raylib::Texture2D& get(const std::string& name) {
        return impl.get(name);
    }

    [[nodiscard]] const raylib::Texture2D& get(const std::string& name) const {
        return impl.get(name);
    }

    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }

   private:
    struct TextureLibraryImpl : Library<raylib::Texture2D> {
        virtual raylib::Texture2D convert_filename_to_object(
            const char*, const char* filename) override {
            return raylib::LoadTexture(filename);
        }

        virtual void unload(raylib::Texture2D texture) override {
            raylib::UnloadTexture(texture);
        }
    } impl;
};
