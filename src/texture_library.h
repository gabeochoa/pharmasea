
#pragma once

#include "external_include.h"
//
#include "files.h"
#include "singleton.h"

SINGLETON_FWD(TextureLibrary)
struct TextureLibrary {
    SINGLETON(TextureLibrary)

    std::map<std::string, Texture2D> textures;

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
        this->add(name, LoadTexture(filename));
    }

    const std::string add(const char* name, const Texture& texture) {
        if (textures.find(name) != textures.end()) {
            // log_warn(
            // "Failed to add texture to library, texture with name {} "
            // "already exists",
            // name);
            return "";
        }
        // log_trace("Adding Texture \"{}\" to our library", name);
        textures[name] = texture;
        return name;
    }

    Texture& get(const std::string& name) { return textures[name]; }

    bool contains(const std::string& name) {
        return (textures.find(name) != textures.end());
    }
};
