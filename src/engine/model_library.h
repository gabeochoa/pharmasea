
#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(ModelLibrary)
struct ModelLibrary {
    SINGLETON(ModelLibrary)

    struct ModelLibraryImpl : Library<Model> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading model: {} from {}", name, filename);
            this->add(name, LoadModel(filename));
        }
        virtual void unload(Model model) override { UnloadModel(model); }
    } impl;

    Model& get(const std::string& name) { return impl.get(name); }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
