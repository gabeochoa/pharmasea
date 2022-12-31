#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
    SINGLETON(ShaderLibrary)

    struct ShaderLibraryImpl : Library<Shader> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading shader: {} from {}", name, filename);
            // TODO null first param sets default vertex shader, do we want
            // this?
            this->add(name, LoadShader(0, filename));
        }

        virtual void unload(Shader shader) override { UnloadShader(shader); }
    } impl;

    [[nodiscard]] Shader& get(const std::string& name) {
        return impl.get(name);
    }
    [[nodiscard]] const Shader& get(const std::string& name) const {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
