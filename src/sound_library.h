

#pragma once

#include "external_include.h"
//
#include "files.h"
#include "singleton.h"

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
    SINGLETON(SoundLibrary)

    std::map<std::string, raylib::Sound> sounds;

    auto size() { return sounds.size(); }
    auto begin() { return sounds.begin(); }
    auto end() { return sounds.end(); }

    auto begin() const { return sounds.begin(); }
    auto end() const { return sounds.end(); }

    auto rbegin() const { return sounds.rbegin(); }
    auto rend() const { return sounds.rend(); }

    auto rbegin() { return sounds.rbegin(); }
    auto rend() { return sounds.rend(); }

    auto empty() const { return sounds.empty(); }

    void load(const char* filename, const char* name) {
        // TODO add debug mode
        std::cout << "loading sound: " << name << " from " << filename
                  << std::endl;
        this->add(name, raylib::LoadSound(filename));
    }

    const std::string add(const char* name, const raylib::Sound& sound) {
        if (sounds.find(name) != sounds.end()) {
            // log_warn(
            // "Failed to add sound to library, sound with name {} "
            // "already exists",
            // name);
            return "";
        }
        // log_trace("Adding Sound \"{}\" to our library", name);
        sounds[name] = sound;
        return name;
    }

    raylib::Sound& get(const std::string& name) {
        if (!this->contains(name)) {
            std::cout << "asking for sound: " << name
                      << " but nothing has been loaded with that name yet"
                      << std::endl;
        }
        return sounds[name];
    }

    bool contains(const std::string& name) {
        return (sounds.find(name) != sounds.end());
    }
};
