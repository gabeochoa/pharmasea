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
            const char* vertex_fn, const char* frag_fn) override {
            return raylib::LoadShader(vertex_fn, frag_fn);
        }

        virtual void unload(raylib::Shader shader) override {
            raylib::UnloadShader(shader);
        }
    } impl;
};
