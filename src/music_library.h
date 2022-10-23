#pragma once

#include "external_include.h"
//
#include "files.h"
#include "singleton.h"

SINGLETON_FWD(MusicLibrary)
struct MusicLibrary {
    SINGLETON(MusicLibrary)

    std::map<std::string, raylib::Music> musics;

    auto size() { return musics.size(); }
    auto begin() { return musics.begin(); }
    auto end() { return musics.end(); }

    auto begin() const { return musics.begin(); }
    auto end() const { return musics.end(); }

    auto rbegin() const { return musics.rbegin(); }
    auto rend() const { return musics.rend(); }

    auto rbegin() { return musics.rbegin(); }
    auto rend() { return musics.rend(); }

    auto empty() const { return musics.empty(); }

    void load(const char* filename, const char* name) {
        // TODO add debug mode
        std::cout << "loading music: " << name << " from " << filename
                  << std::endl;
        this->add(name, raylib::LoadMusicStream(filename));
    }

    const std::string add(const char* name, const raylib::Music& music) {
        if (musics.find(name) != musics.end()) {
            // log_warn(
            // "Failed to add music to library, music with name {} "
            // "already exists",
            // name);
            return "";
        }
        // log_trace("Adding music \"{}\" to our library", name);
        musics[name] = music;
        return name;
    }

    raylib::Music& get(const std::string& name) {
        if (!this->contains(name)) {
            std::cout << "asking for music: " << name
                      << " but nothing has been loaded with that name yet"
                      << std::endl;
        }
        return musics[name];
    }

    bool contains(const std::string& name) {
        return (musics.find(name) != musics.end());
    }
};
