#pragma once

#include "../ah.h"
#include "singleton.h"

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
    SINGLETON(ShaderLibrary)

    [[nodiscard]] raylib::Shader& get(const std::string& name) {
        return impl.get(name);
    }
    [[nodiscard]] const raylib::Shader& get(const std::string& name) const {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }

    // Load a shader with explicit vertex + fragment files.
    void load(const char* vs_filename, const char* fs_filename,
              const char* name) {
        // Store directly by name without any filename encoding.
        // If re-loading the same name, unload the previous shader first.
        if (impl.contains(name)) {
            raylib::UnloadShader(impl.get(name));
        }
        impl.storage[name] = raylib::LoadShader(vs_filename, fs_filename);
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct ShaderLibraryImpl : afterhours::Library<raylib::Shader> {
        virtual raylib::Shader convert_filename_to_object(
            const char* name, const char* filename) override {
            // Use the library key (first param) as the vertex shader filename.
            // The library "filename" is treated as the fragment shader filename.
            // NOTE: This path is not used by our current preload (we load via the
            // explicit (vs, fs, name) overload), but keeping it consistent helps
            // if someone uses impl.load(...) directly.
            return raylib::LoadShader(name, filename);
        }

        virtual void unload(raylib::Shader shader) override {
            raylib::UnloadShader(shader);
        }
    } impl;
};
