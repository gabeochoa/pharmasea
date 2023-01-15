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
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading shader: {} from {}", name, filename);
            // TODO null first param sets default vertex shader, do we want
            // this?
            this->add(name, raylib::LoadShader(0, filename));
        }

        virtual void unload(raylib::Shader shader) override {
            raylib::UnloadShader(shader);
        }
    } impl;
};
