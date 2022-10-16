
#pragma once

#include "external_include.h"
//
#include "files.h"
#include "singleton.h"

SINGLETON_FWD(ModelLibrary)
struct ModelLibrary {
    SINGLETON(ModelLibrary)

    std::map<std::string, Model> textures;

    auto size() { return textures.size(); }
    auto begin() { return textures.begin(); }
    auto end() { return textures.end(); }

    auto begin() const { return textures.begin(); }
    auto end() const { return textures.end(); }

    auto rbegin() const { return textures.rbegin(); }
    auto rend() const { return textures.rend(); }

    auto rbegin() { return textures.rbegin(); }
    auto rend() { return textures.rend(); }

    auto empty() const { return textures.empty(); }

    void load(const char* filename, const char* name) {
        // TODO add debug mode
        std::cout << "loading texture: " << name << " from " << filename
                  << std::endl;
        this->add(name, LoadModel(filename));
    }

    const std::string add(const char* name, const Model& texture) {
        if (textures.find(name) != textures.end()) {
            // log_warn(
            // "Failed to add texture to library, texture with name {} "
            // "already exists",
            // name);
            return "";
        }
        // log_trace("Adding Model \"{}\" to our library", name);
        textures[name] = texture;
        return name;
    }

    Model& get(const std::string& name) {
        if (!this->contains(name)) {
            std::cout << "asking for texture: " << name
                      << " but nothing has been loaded with that name yet"
                      << std::endl;
        }
        return textures[name];
    }

    bool contains(const std::string& name) {
        return (textures.find(name) != textures.end());
    }
};
