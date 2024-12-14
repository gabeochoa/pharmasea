
#pragma once

// Note move to cpp if we create one
#include "files.h"
//
#include "library.h"
#include "singleton.h"

constexpr int MAX_ANIM_NAME_LENGTH = 100;

struct AnimationInfo {
    // Note this has to be a string because the string_view isnt serializable in
    // bitsery
    std::string anim_name;
    int index;

    int frame;
    int totalFrames;

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(anim_name, index, frame, totalFrames);
    }
};

SINGLETON_FWD(AnimLibrary)
struct AnimLibrary {
    SINGLETON(AnimLibrary)
    struct Animation {
        unsigned int anims;
        raylib::ModelAnimation* anim_ptr;
    };

    struct AnimLoadingInfo {
        const char* folder;
        const char* filename;
        const char* libraryname;
        const char* texture;
    };

    [[nodiscard]] const Animation& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] Animation& get(const std::string& name) {
        return impl.get(name);
    }

    void load(const AnimLoadingInfo& mli) {
        const auto full_filename =
            Files::get().fetch_resource_path(mli.folder, mli.filename);
        impl.load(full_filename.c_str(), mli.libraryname);
        if (strlen(mli.libraryname) > MAX_ANIM_NAME_LENGTH) {
            log_warn(
                "Loadded animation {} but name is longer than our max length "
                "which means there might be conflicts on serialization",
                mli.libraryname);
        }
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct AnimLibraryImpl : Library<Animation> {
        virtual Animation convert_filename_to_object(
            const char*, const char* filename) override {
            unsigned int temp_num_anims;
            raylib::ModelAnimation* ma =
                raylib::LoadModelAnimations(filename, &temp_num_anims);
            return Animation{.anims = temp_num_anims, .anim_ptr = ma};
        }

        virtual void unload(Animation anim) override {
            raylib::UnloadModelAnimation(*(anim.anim_ptr));
        }
    } impl;
};
