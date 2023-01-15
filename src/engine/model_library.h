
#pragma once

// Note move to cpp if we create one
#include "files.h"
//
#include "library.h"
#include "singleton.h"

struct ModelInfo {
    raylib::Model model;
    float size_scale;
    vec3 position_offset;
    // TODO it would be nice to support this,
    // but we have no way of doing a double rotation on diff axis
    // vec3 rotation_axis = vec3{0, 1, 0};
    float rotation_angle = 0;
};

SINGLETON_FWD(ModelLibrary)
struct ModelLibrary {
    SINGLETON(ModelLibrary)

    struct ModelLoadingInfo {
        const char* folder;
        const char* filename;
        const char* libraryname;
        const char* texture;
    };

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

    void load(ModelLoadingInfo mli) {
        const auto full_filename =
            Files::get().fetch_resource_path(mli.folder, mli.filename);
        impl.load(full_filename.c_str(), mli.libraryname);
    }
};
