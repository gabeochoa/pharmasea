

#pragma once

//
#include <raylib/glfw3.h>

#include <algorithm>
#include <fstream>

#include "../external_include.h"
//

#include "app.h"
#include "event.h"
#include "files.h"
#include "globals.h"
#include "log.h"
#include "music_library.h"
#include "resolution.h"
#include "singleton.h"
#include "sound_library.h"
#include "util.h"
// TODO we should not be reaches outside of engine
#include "../strings.h"

namespace settings {

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

// TODO how do we support different games having different save file data
// requirements?

const int MAX_LANG_LENGTH = 25;

// TODO How do we support multiple versions
// we dont want to add a new field and break
// all past save games
// we need some way to only parse based on the version in the save file
// https://developernote.com/2020/02/basic-ideas-of-version-tolerant-serialization-in-cpp/
struct Data {
    int engineVersion = 0;
    rez::ResolutionInfo resolution = {.width = 1280, .height = 720};
    std::string lang_name = "en_us";
    // Volume percent [0, 1] for everything
    float master_volume = 0.5f;
    float music_volume = 0.5f;
    float sound_volume = 0.5f;
    std::string username;
    std::string last_ip_joined;
    std::string ui_theme;
    bool show_streamer_safe_box = false;
    bool enable_postprocessing = true;
    bool snapCameraTo90 = false;
    bool isFullscreen = false;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(engineVersion);
        s.value4b(master_volume);
        s.value4b(music_volume);
        s.value4b(sound_volume);

        s.value1b(show_streamer_safe_box);
        s.value1b(snapCameraTo90);
        s.value1b(enable_postprocessing);
        s.value1b(isFullscreen);

        s.text1b(username, network::MAX_NAME_LENGTH);
        s.text1b(last_ip_joined, 25);
        s.text1b(lang_name, MAX_LANG_LENGTH);
        s.text1b(ui_theme, MAX_LANG_LENGTH);

        s.object(resolution);
    }
    friend std::ostream& operator<<(std::ostream& os, const Data& data) {
        os << "Settings(" << std::endl;
        os << "version: " << data.engineVersion << std::endl;
        os << "resolution: " << data.resolution.width << ", "
           << data.resolution.height << std::endl;
        os << "fullscreen: " << data.isFullscreen << std::endl;
        os << "master vol: " << data.master_volume << std::endl;
        os << "music vol: " << data.music_volume << std::endl;
        os << "sound vol: " << data.sound_volume << std::endl;
        os << "Safe box: " << data.show_streamer_safe_box << std::endl;
        os << "username: " << data.username << std::endl;
        os << "post_processing: " << data.enable_postprocessing << std::endl;
        os << "last ip joined: " << data.last_ip_joined << std::endl;
        os << "lang name: " << data.lang_name << std::endl;
        os << "ui_theme: " << data.ui_theme << std::endl;
        os << "should snap camera: " << data.snapCameraTo90 << std::endl;
        os << ")" << std::endl;
        return os;
    }
};

// TODO move into its own file?

struct LanguageInfo {
    std::string name;
    std::string filename;

    bool operator<(const LanguageInfo& r) const { return name < r.name; }
    bool operator==(const LanguageInfo& r) const { return name == r.name; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(name, MAX_LANG_LENGTH);
        s.text1b(filename, MAX_LANG_LENGTH);
    }
};

}  // namespace settings

SINGLETON_FWD(Settings)
struct Settings {
    SINGLETON(Settings)

    settings::Data data;

    std::vector<settings::LanguageInfo> lang_options;

    Settings() : data(settings::Data()) {}

    ~Settings() {}

    void reset_to_default() {
        data = settings::Data();
        refresh_settings();
    }

    void update_resolution_from_index(int index) {
        update_window_size(rez::ResolutionExplorer::get().fetch(index));
    }

    void update_window_size(rez::ResolutionInfo rez) {
        data.resolution = rez;

        data.resolution.width = static_cast<int>(
            fminf(3860.f, fmaxf((float) data.resolution.width, 1280.f)));

        data.resolution.height = static_cast<int>(
            fminf(2160.f, fmaxf((float) data.resolution.height, 720.f)));

        //
        WindowResizeEvent* event = new WindowResizeEvent(
            data.resolution.width, data.resolution.height);

        __WIN_W = data.resolution.width;
        __WIN_H = data.resolution.height;

        App::get().processEvent(*event);
        delete event;
    }

    void update_fullscreen(bool fs_enabled) {
        data.isFullscreen = fs_enabled;
        WindowFullscreenEvent* event = new WindowFullscreenEvent(fs_enabled);
        App::get().processEvent(*event);
    }

    void toggle_fullscreen() { update_fullscreen(!data.isFullscreen); }

    void update_last_used_ip_address(const std::string& ip) {
        data.last_ip_joined = ip;
    }

    void update_master_volume(float nv) {
        data.master_volume = util::clamp(nv, 0.f, 1.f);
        log_info("master volume changed to {}", data.master_volume);

        MusicLibrary::get().update_volume(data.music_volume);
        SoundLibrary::get().update_volume(data.sound_volume);

        raylib::SetMasterVolume(data.master_volume);
    }

    void update_music_volume(float nv) {
        data.music_volume = util::clamp(nv, 0.f, 1.f);
        MusicLibrary::get().update_volume(data.music_volume);
    }

    void update_sound_volume(float nv) {
        data.sound_volume = util::clamp(nv, 0.f, 1.f);
        SoundLibrary::get().update_volume(data.sound_volume);
    }

    void update_streamer_safe_box(bool sssb) {
        data.show_streamer_safe_box = sssb;
    }

    void update_post_processing_enabled(bool pp_enabled) {
        data.enable_postprocessing = pp_enabled;
    }
    [[nodiscard]] int get_current_resolution_index() const {
        return rez::ResolutionExplorer::get().index(data.resolution);
    }

    [[nodiscard]] std::vector<std::string> resolution_options() const {
        return rez::ResolutionExplorer::get().fetch_options();
    }

    [[nodiscard]] std::string last_used_ip() const {
        return data.last_ip_joined;
    }

    // TODO these could be private and inside the ctor/dtor with RAII if we are
    // okay with running on get() and ignoring the result

    // TODO music volumes only seem to take effect once you open SettingsLayer
    bool load_save_file() {
        std::ifstream ifs(Files::get().settings_filepath());
        if (!ifs.is_open()) {
            log_warn("Failed to find settings file (Read) {}",
                     Files::get().settings_filepath());
            return false;
        }

        log_trace("Reading settings file");
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        auto buf_str = buffer.str();

        bitsery::quickDeserialization<settings::InputAdapter>(
            {buf_str.begin(), buf_str.size()}, data);

        refresh_settings();

        log_info("Settings Loaded: {}", data);
        log_trace("End loading settings file");
        ifs.close();
        return true;
    }

    // TODO instead of writing to a string and then file
    // theres a way to write directly to the file
    // https://github.com/fraillt/bitsery/blob/master/examples/file_stream.cpp
    bool write_save_file() {
        std::ofstream ofs(Files::get().settings_filepath());
        if (!ofs.is_open()) {
            log_warn("Failed to find settings file (Write) {}",
                     Files::get().settings_filepath());
            return false;
        }
        settings::Buffer buffer;
        bitsery::quickSerialization(settings::OutputAdapter{buffer}, data);
        ofs << buffer << std::endl;
        ofs.close();

        log_info("Wrote Settings File to {}", Files::get().settings_filepath());
        return true;
    }

    void load_language_options() {
        if (!lang_options.empty()) return;

        Files::get().for_resources_in_group(
            strings::settings::TRANSLATIONS,
            [&](const std::string& name, const std::string& filename,
                const std::string& extension) {
                if (extension != ".mo") return;
                log_info("adding {} {} {}", name, filename, extension);

                lang_options.push_back(
                    settings::LanguageInfo{.name = name, .filename = filename});
            });

        if (lang_options.empty()) {
            // TODO Fail more gracefully in the future
            log_error("no language options found");
        }

        log_info("loaded {} language options", lang_options.size());
    }

    [[nodiscard]] std::vector<std::string> language_options() {
        std::vector<std::string> options;
        options.reserve(lang_options.size());
        for (const auto& li : lang_options) {
            options.push_back(li.name);
        }
        return options;
    }

    [[nodiscard]] int get_current_language_index() {
        auto _index = [&](const std::string& name) -> int {
            int i = 0;
            for (const auto& res : lang_options) {
                if (res.name == name) return i;
                i++;
            }
            return -1;
        };
        return _index(data.lang_name);
    }

    void update_language_from_index(int index);

    void update_language_name(const std::string& l) {
        std::string lang = l;
        if (lang.empty()) lang = lang_options[0].name;

        auto _index = [&](const std::string& name) -> int {
            int i = 0;
            for (const auto& res : lang_options) {
                if (res.name == name) return i;
                i++;
            }
            return -1;
        };
        update_language_from_index(_index(lang));
    }

    void update_ui_theme(const std::string& s);
    std::vector<std::string> get_ui_theme_options();
    int get_ui_theme_selected_index();
    void update_theme_from_index(int index);

    // Note: Basically once we load the file,
    // we run into an issue where our settings is correct,
    // but the underlying data isnt being used
    //
    // This function is used by the load to kick raylib into
    // the right config
    void refresh_settings() {
        // Force a resolution fetch so that after the settings loads we have
        // them ready
        rez::ResolutionExplorer::get().load_resolution_options();

        // Refresh the language files
        load_language_options();

        // version doesnt need update
        update_window_size(data.resolution);
        update_music_volume(data.music_volume);
        update_sound_volume(data.sound_volume);
        update_master_volume(data.master_volume);
        update_streamer_safe_box(data.show_streamer_safe_box);
        update_language_name(data.lang_name);
        update_ui_theme(data.ui_theme);
        update_fullscreen(data.isFullscreen);
    }
};
