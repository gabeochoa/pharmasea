#pragma once

#include "external_include.h"
//
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(ShaderLibrary)
struct ShaderLibrary {
    SINGLETON(ShaderLibrary)

    struct ShaderLibraryImpl : Library<Shader> {
        virtual void load(const char* filename, const char* name) override {
            // TODO add debug mode
            std::cout << "loading Shader : " << name << " from " << filename
                      << std::endl;
            // TODO null first param sets default vertex shader, do we want
            // this?
            this->add(name, LoadShader(0, filename));
        }
    } impl;

    Shader& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
