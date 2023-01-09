
#pragma once

#include "library.h"
#include "singleton.h"

SINGLETON_FWD(ModelLibrary)
struct ModelLibrary {
    SINGLETON(ModelLibrary)

    struct ModelLibraryImpl : Library<raylib::Model> {
        virtual void load(const char* filename, const char* name) override {
            log_info("Loading model: {} from {}", name, filename);
            this->add(name, raylib::LoadModel(filename));
        }
        virtual void unload(raylib::Model model) override {
            raylib::UnloadModel(model);
        }
    } impl;

    [[nodiscard]] const raylib::Model& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] raylib::Model& get(const std::string& name) {
        return impl.get(name);
    }
    void load(const char* filename, const char* name) {
        impl.load(filename, name);
    }
};
