
#pragma once

#include "../../strings.h"
#include "afterhours/src/plugins/sound_system.h"

// Helper wrappers for cleaner sound/music API access
// Usage: Sounds::play(strings::sounds::SoundId::CLICK)
//        Music::play(strings::music::THEME)

struct Sounds {
    static afterhours::sound_system::SoundLibrary& get() {
        return afterhours::sound_system::SoundLibrary::get();
    }

    static void play(strings::sounds::SoundId id) {
        get().play(strings::sounds::to_name(id));
    }

    static void play(const char* name) { get().play(name); }

    static void play_random_match(const std::string& prefix) {
        get().play_random_match(prefix);
    }
};

struct Music {
    static afterhours::sound_system::MusicLibrary& get() {
        return afterhours::sound_system::MusicLibrary::get();
    }

    // TODO music stops playing when you grab title bar
    static void play(const std::string& name) {
        auto& m = get().get(name);
        if (!raylib::IsMusicStreamPlaying(m)) {
            afterhours::PlayMusicStream(m);
        }
    }

    static raylib::Music& stream(const std::string& name) {
        return get().get(name);
    }
};

namespace ui {

namespace sounds {
inline void select() { Sounds::play(strings::sounds::SoundId::SELECT); }
inline void click() { Sounds::play(strings::sounds::SoundId::CLICK); }
}  // namespace sounds

}  // namespace ui
