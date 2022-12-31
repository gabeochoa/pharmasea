

#pragma once

//
#include <raylib/glfw3.h>

#include <algorithm>

#include "../external_include.h"
//

#include "../util.h"
#include "app.h"
#include "event.h"
#include "files.h"
#include "globals.h"
#include "log.h"
#include "music_library.h"
#include "resolution.h"
#include "singleton.h"

namespace settings {

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

// TODO how do we support different games having different save file data
// requirements?

// TODO How do we support multiple versions
// we dont want to add a new field and break
// all past save games
// we need some way to only parse based on the version in the save file
// https://developernote.com/2020/02/basic-ideas-of-version-tolerant-serialization-in-cpp/
struct Data {
    int engineVersion = 0;
    rez::ResolutionInfo resolution;
    // Volume percent [0, 1] for everything
    float master_volume = 0.5f;
    float music_volume = 0.5f;
    bool show_streamer_safe_box = false;
    std::string username = "";
    bool enable_postprocessing = true;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(engineVersion);
        s.object(resolution);
        s.value4b(master_volume);
        s.value4b(music_volume);
        s.value1b(show_streamer_safe_box);
        s.text1b(username, network::MAX_NAME_LENGTH);
        s.value1b(enable_postprocessing);
    }
    friend std::ostream& operator<<(std::ostream& os, const Data& data) {
        os << "Settings(" << std::endl;
        os << "version: " << data.engineVersion << std::endl;
        os << "resolution: " << data.resolution.width << ", "
           << data.resolution.height << std::endl;
        os << "master vol: " << data.master_volume << std::endl;
        os << "music vol: " << data.music_volume << std::endl;
        os << "Safe box: " << data.show_streamer_safe_box << std::endl;
        os << "username: " << data.username << std::endl;
        os << "post_processing: " << data.enable_postprocessing << std::endl;
        os << ")" << std::endl;
        return os;
    }
};

}  // namespace settings

SINGLETON_FWD(Settings)
struct Settings {
    SINGLETON(Settings)

    settings::Data data;

    Settings() {}

    ~Settings() {}

    void update_resolution_from_index(int index) {
        update_window_size(rez::ResolutionExplorer::get().fetch(index));
    }

    void update_window_size(rez::ResolutionInfo rez) {
        data.resolution = rez;

        data.resolution.width = static_cast<int>(
            fminf(3860.f, fmaxf(data.resolution.width, 1280.f)));

        data.resolution.height = static_cast<int>(
            fminf(2160.f, fmaxf(data.resolution.height, 720.f)));

        //
        WindowResizeEvent* event = new WindowResizeEvent(
            data.resolution.width, data.resolution.height);

        __WIN_W = data.resolution.width;
        __WIN_H = data.resolution.height;

        App::get().processEvent(*event);
        delete event;
    }

    void update_master_volume(float nv) {
        data.master_volume = util::clamp(nv, 0.f, 1.f);
        log_trace("master volume changed to {}", data.master_volume);
        SetMasterVolume(data.master_volume);
    }

    void update_music_volume(float nv) {
        data.music_volume = util::clamp(nv, 0.f, 1.f);
        MusicLibrary::get().update_volume(data.music_volume);
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

    // TODO these could be private and inside the ctor/dtor with RAII if we are
    // okay with running on get() and ignoring the result

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

        update_all_settings();

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

   private:
    // Basically once we load the file,
    // we run into an issue where our settings is correct,
    // but the underlying data isnt being used
    //
    // This function is used by the load to kick raylib into
    // the right config
    void update_all_settings() {
        // Force a resolution fetch so that after the settings loads we have
        // them ready
        rez::ResolutionExplorer::get().load_resolution_options();

        // version doesnt need update
        update_window_size(data.resolution);
        update_master_volume(data.master_volume);
        update_music_volume(data.music_volume);
        update_streamer_safe_box(data.show_streamer_safe_box);
    }
};
