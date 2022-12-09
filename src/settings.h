
#pragma once

//
#include <raylib/glfw3.h>

#include <algorithm>

#include "external_include.h"
//

#include "app.h"
#include "constexpr_containers.h"
#include "event.h"
#include "files.h"
#include "globals.h"
#include "singleton.h"
#include "util.h"

namespace settings {
struct ResolutionInfo {
    int width;
    int height;

    bool operator<(const ResolutionInfo& r) const {
        return (this->width < r.width) ||
               ((this->width == r.width) && (this->height < r.height));
    }

    bool operator==(const ResolutionInfo& r) const {
        return (this->width == r.width) && (this->height == r.height);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(this->width);
        s.value4b(this->height);
    }
};

static std::vector<ResolutionInfo> RESOLUTION_OPTIONS;
static std::vector<std::string> STRING_RESOLUTION_OPTIONS;

using Buffer = std::string;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

// TODO How do we support multiple versions
// we dont want to add a new field and break
// all past save games
// we need some way to only parse based on the version in the save file
// https://developernote.com/2020/02/basic-ideas-of-version-tolerant-serialization-in-cpp/
struct Data {
    int version = 0;
    ResolutionInfo resolution;
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
        s.value4b(version);
        s.object(resolution);
        s.value4b(master_volume);
        s.value4b(music_volume);
        s.value1b(show_streamer_safe_box);
        s.text1b(username, network::MAX_NAME_LENGTH);
        s.value1b(enable_postprocessing);
    }
    friend std::ostream& operator<<(std::ostream& os, const Data& data) {
        os << "Settings(" << std::endl;
        os << "version: " << data.version << std::endl;
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

    // Basically once we load the file,
    // we run into an issue where our settings is correct,
    // but the underlying data isnt being used
    //
    // This function is used by the load to kick raylib into
    // the right config
    void update_all_settings() {
        // version doesnt need update
        update_window_size(data.resolution);
        update_master_volume(data.master_volume);
        update_music_volume(data.music_volume);
        update_streamer_safe_box(data.show_streamer_safe_box);
    }

    void update_resolution_from_index(int index) {
        update_window_size(settings::RESOLUTION_OPTIONS[index]);
    }

    void update_window_size(settings::ResolutionInfo rez) {
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
        // std::cout << "master volume changed to " << data.master_volume
        // << std::endl;
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

    void load_resolution_options() {
#ifdef __APPLE__
        // Nothing this one works :) 
 #else
        // TODO either implement these for windows or get them in the dll 
        const auto glfwGetPrimaryMonitor = []() -> GLFWmonitor* {
            return nullptr;
        };
        const auto glfwGetVideoModes = [](GLFWmonitor*, int*) -> GLFWvidmode* {
            return nullptr;
        };
#endif 
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        int count = 0;
        const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);

        for (int i = 0; i < count; i++) {
            GLFWvidmode mode = modes[i];
            // int width
            // int height
            // int redBits int greenBits int blueBits
            // int refreshRate

            // Just kinda easier to not support every possible resolution
            if (mode.height < 720 || mode.height > 2160) continue;

            settings::RESOLUTION_OPTIONS.push_back(settings::ResolutionInfo{
                .width = mode.width, .height = mode.height});
        }
        
        if (settings::RESOLUTION_OPTIONS.empty()) {
            settings::RESOLUTION_OPTIONS.push_back(
                settings::ResolutionInfo{.width = 1280, .height = 720});
            settings::RESOLUTION_OPTIONS.push_back(
                settings::ResolutionInfo{.width = 1920, .height = 1080});
            settings::RESOLUTION_OPTIONS.push_back(
                settings::ResolutionInfo{.width = 3860, .height = 2160}); 
        }

        // TODO SPEED this kinda slow but it only happens once
        settings::RESOLUTION_OPTIONS.erase(
            std::unique(settings::RESOLUTION_OPTIONS.begin(),
                        settings::RESOLUTION_OPTIONS.end()),
            settings::RESOLUTION_OPTIONS.end());
    }

    void convert_res_options_to_text() {
        std::transform(settings::RESOLUTION_OPTIONS.cbegin(),
                       settings::RESOLUTION_OPTIONS.cend(),
                       std::back_inserter(settings::STRING_RESOLUTION_OPTIONS),
                       [](settings::ResolutionInfo info) {
                           return fmt::format("{}x{}", info.width, info.height);
                       });
    }

    std::vector<std::string> resolution_options() {
        if (settings::RESOLUTION_OPTIONS.empty()) load_resolution_options();
        if (settings::STRING_RESOLUTION_OPTIONS.empty())
            convert_res_options_to_text();
        return settings::STRING_RESOLUTION_OPTIONS;
    }

    bool load_save_file() {
        std::ifstream ifs(Files::get().settings_filepath());
        if (!ifs.is_open()) {
            std::cout << ("failed to find settings file (read)") << std::endl;
            return false;
        }

        std::cout << "reading settings file" << std::endl;
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        auto buf_str = buffer.str();

        bitsery::quickDeserialization<settings::InputAdapter>(
            {buf_str.begin(), buf_str.size()}, data);

        update_all_settings();

        std::cout << "Finished loading: " << std::endl;
        std::cout << data << std::endl;
        std::cout << "end settings file" << std::endl;
        ifs.close();
        return true;
    }

    // TODO instead of writing to a string and then file
    // theres a way to write directly to the file
    // https://github.com/fraillt/bitsery/blob/master/examples/file_stream.cpp
    bool write_save_file() {
        std::ofstream ofs(Files::get().settings_filepath());
        if (!ofs.is_open()) {
            std::cout << ("failed to find settings file (write)") << std::endl;
            return false;
        }
        settings::Buffer buffer;
        bitsery::quickSerialization(settings::OutputAdapter{buffer}, data);
        std::string line;
        ofs << buffer << std::endl;
        ofs.close();

        std::cout << "wrote settings file to "
                  << Files::get().settings_filepath() << std::endl;
        return true;
    }
};
