#pragma once

#include "../ah.h"
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

    // Load a shader with explicit vertex + fragment files.
    // Implementation note: we store "vs|fs" in the underlying filename field.
    void load(const char* vs_filename, const char* fs_filename,
              const char* name) {
        std::string encoded = std::string(vs_filename) + "|" + fs_filename;
        impl.load(encoded.c_str(), name);
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct ShaderLibraryImpl : afterhours::Library<raylib::Shader> {
        virtual raylib::Shader convert_filename_to_object(
            const char*, const char* filename) override {
            // Support either:
            // - fragment-only: "file.fs" (uses raylib default vertex shader)
            // - explicit pair: "file.vs|file.fs"
            const std::string f(filename);
            const auto split = f.find('|');
            if (split == std::string::npos) {
                // TODO null first param sets default vertex shader, do we want this?
                return raylib::LoadShader(0, filename);
            }
            const std::string vs = f.substr(0, split);
            const std::string fs = f.substr(split + 1);
            return raylib::LoadShader(vs.c_str(), fs.c_str());
        }

        virtual void unload(raylib::Shader shader) override {
            raylib::UnloadShader(shader);
        }
    } impl;
};
