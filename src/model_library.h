
#pragma once

#include "external_include.h"
//
#include "files.h"
#include "library.h"
#include "singleton.h"

SINGLETON_FWD(ModelLibrary)
struct ModelLibrary {
    SINGLETON(ModelLibrary)

    struct ModelLibraryImpl : Library<Model> {
        virtual void load(const char* filename, const char* name) override {
            // TODO add debug mode
            std::cout << "loading texture: " << name << " from " << filename
                      << std::endl;
            this->add(name, LoadModel(filename));
        }
    } impl;

    Model& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
