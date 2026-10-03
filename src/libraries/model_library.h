
#pragma once

// Note move to cpp if we create one
#include <algorithm>
#include <string>

#include "../engine/files.h"
//
#include "../ah.h"
#include "../engine/gltf_loader.h"
#include "../engine/graphics.h"
#include "../engine/singleton.h"

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
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(            //
            self.model_name,       //
            self.size_scale,       //
            self.position_offset,  //
            self.rotation_angle    //
        );
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

    [[nodiscard]] bool contains(const std::string& name) const {
        return impl.contains(name);
    }

    void load(ModelLoadingInfo mli) {
        const auto full_filename =
            Files::get().fetch_resource_path(mli.folder, mli.filename);
        impl.load(full_filename.c_str(), mli.libraryname);
    }

    // Lazy loading support
    void register_lazy_model(const std::string& name, ModelLoadingInfo info) {
        lazy_model_configs[name] = info;
    }
    void ensure_loaded(const std::string& name) {
        if (!impl.contains(name)) {
            auto it = lazy_model_configs.find(name);
            if (it != lazy_model_configs.end()) {
                log_info("Loading lazy model {} on-demand", name);
                load(it->second);
            }
        }
    }
    [[nodiscard]] raylib::Model get_and_load_if_needed(
        const std::string& name) {
        ensure_loaded(name);
        return get(name);
    }

    void unload_all() { impl.unload_all(); }
    [[nodiscard]] auto size() { return impl.size(); }

   private:
    // Storage for lazy-loaded model configurations
    std::unordered_map<std::string, ModelLoadingInfo> lazy_model_configs;
    struct ModelLibraryImpl : afterhours::Library<raylib::Model> {
        virtual raylib::Model convert_filename_to_object(
            const char*, const char* filename) override {
            std::string path(filename);
            std::string lower = path;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           ::tolower);

            const bool is_gltf =
                lower.ends_with(".gltf") || lower.ends_with(".glb");

            if (is_gltf) {
                std::string warn;
                std::string err;
                auto loaded = gltf_loader::load_model(path, warn, err);
                if (!warn.empty()) {
                    log_warn("gltf warning {}: {}", filename, warn);
                }
                if (loaded.has_value()) {
                    log_info("Loaded GLTF model {}", filename);
                    return loaded.value();
                }
                log_warn("gltf load failed for {}: {} — using fallback cube",
                         filename, err);
            }

            auto model = raylib::LoadModel(filename);
            const bool looksInvalid =
                (model.meshes == nullptr) || (model.meshCount <= 0);
            if (looksInvalid) {
                log_warn("LoadModel failed for {} — returning fallback cube",
                         filename);
                auto mesh = raylib::GenMeshCube(1.0f, 1.0f, 1.0f);
                auto fallback = raylib::LoadModelFromMesh(mesh);
                return fallback;
            }
            return model;
        }

        virtual void unload(raylib::Model model) override {
            raylib::UnloadModel(model);
        }
    } impl;
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

    [[nodiscard]] const ModelInfo& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] ModelInfo& get(const std::string& name) {
        return impl.get(name);
    }

    void load(const ModelLoadingInfo& mli) {
        const auto full_filename =
            Files::get().fetch_resource_path(mli.folder, mli.filename);
        impl.load(full_filename.c_str(), mli.library_name.c_str());

        ModelInfo& mi = get(mli.library_name);
        mi.size_scale = mli.size_scale;
        mi.position_offset = mli.position_offset;
        mi.rotation_angle = mli.rotation_angle;
    }

    void unload_all() { impl.unload_all(); }

    [[nodiscard]] auto size() { return impl.size(); }

   private:
    struct ModelInfoLibraryImpl : afterhours::Library<ModelInfo> {
        virtual ModelInfo convert_filename_to_object(const char* name,
                                                     const char*) override {
            return ModelInfo{
                .model_name = name,                //
                .size_scale = 0.f,                 //
                .position_offset = vec3{0, 0, 0},  //
                .rotation_angle = 0.f,             //
            };
        }

        virtual void unload(ModelInfo) override {}
    } impl;
};
