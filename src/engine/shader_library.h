#pragma once

#include "library.h"
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

    void unload_all() { impl.unload_all(); }

   private:
    struct ShaderLibraryImpl : Library<raylib::Shader> {
        virtual raylib::Shader convert_filename_to_object(
            const char* filename) override {
            // TODO null first param sets default vertex shader, do we want
            // this?
            return raylib::LoadShader(0, filename);
        }

        virtual void unload(raylib::Shader shader) override {
            raylib::UnloadShader(shader);
        }
    } impl;
};
