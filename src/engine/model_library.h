
#pragma once

// Note move to cpp if we create one
#include "files.h"
//
#include "library.h"
#include "raylib.h"
#include "singleton.h"

// TODO enforce it on object creation?
constexpr int MAX_MODEL_NAME_LENGTH = 100;

struct ModelInfo {
    // Note this has to be a string because the string_view isnt serializable in
    // bitsery
    std::string model_name;

    float size_scale;
    vec3 position_offset;
    // TODO it would be nice to support this,
    // but we have no way of doing a double rotation on diff axis
    // vec3 rotation_axis = vec3{0, 1, 0};
    float rotation_angle = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(model_name, MAX_MODEL_NAME_LENGTH);
        s.value4b(size_scale);
        s.object(position_offset);
        s.value4b(rotation_angle);
    }
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

    void unload_all() { impl.unload_all(); }

   private:
    struct ModelLibraryImpl : Library<raylib::Model> {
        virtual raylib::Model convert_filename_to_object(
            const char*, const char* filename) override {
            return raylib::LoadModel(filename);
        }

        virtual void unload(raylib::Model model) override {
            raylib::UnloadModel(model);
        }
    } impl;
};

struct NewModelInfo {
    // Note this has to be a string because the string_view isnt serializable in
    // bitsery
    std::string model_name;

    float size_scale;
    vec3 position_offset;
    // TODO it would be nice to support this,
    // but we have no way of doing a double rotation on diff axis
    // vec3 rotation_axis = vec3{0, 1, 0};
    float rotation_angle = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(model_name, MAX_MODEL_NAME_LENGTH);
        s.value4b(size_scale);
        s.object(position_offset);
        s.value4b(rotation_angle);
    }
};

SINGLETON_FWD(ModelInfoLibrary)
struct ModelInfoLibrary {
    SINGLETON(ModelInfoLibrary)

    struct ModelLoadingInfo {
        std::string folder;
        std::string filename;
        std::string library_name;
        float size_scale;
        vec3 position_offset;
        float rotation_angle;
    };

    [[nodiscard]] bool has(const std::string& name) const {
        return impl.contains(name);
    }

    [[nodiscard]] const NewModelInfo& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] NewModelInfo& get(const std::string& name) {
        return impl.get(name);
    }

    void load(ModelLoadingInfo mli) {
        const auto full_filename =
            Files::get().fetch_resource_path(mli.folder, mli.filename);
        impl.load(full_filename.c_str(), mli.library_name.c_str());

        NewModelInfo& mi = get(mli.library_name);
        mi.size_scale = mli.size_scale;
        mi.position_offset = mli.position_offset;
        mi.rotation_angle = mli.rotation_angle;
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct ModelInfoLibraryImpl : Library<NewModelInfo> {
        virtual NewModelInfo convert_filename_to_object(const char* name,
                                                        const char*) override {
            return NewModelInfo{
                .model_name = name,                //
                .size_scale = 0.f,                 //
                .position_offset = vec3{0, 0, 0},  //
                .rotation_angle = 0.f,             //
            };
        }

        virtual void unload(NewModelInfo) override {}
    } impl;
};
