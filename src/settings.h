
#pragma once

//
#include "external_include.h"
//

#include "app.h"
#include "event.h"
#include "files.h"
#include "globals.h"
#include "singleton.h"
#include "util.h"

namespace bitsery {
template<typename S>
void serialize(S& s, vec2& data) {
    s.value4b(data.x);
    s.value4b(data.y);
}
}  // namespace bitsery

namespace settings {
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
    vec2 window_size = {WIN_W, WIN_H};
    // Volume percent [0, 1] for everything
    float masterVolume = 0.5f;
    bool show_streamer_safe_box = false;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(version);
        s.object(window_size);
        s.value4b(masterVolume);
        s.value1b(show_streamer_safe_box);
    }
};

std::ostream& operator<<(std::ostream& os, const Data& data) {
    os << "Settings(" << std::endl;
    os << "version: " << data.version << std::endl;
    os << "resolution: " << data.window_size.x << ", " << data.window_size.y
       << std::endl;
    os << "master vol: " << data.masterVolume << std::endl;
    os << "Safe box: " << data.show_streamer_safe_box << std::endl;
    return os;
}

}  // namespace settings

SINGLETON_FWD(Settings)
struct Settings {
    SINGLETON(Settings)

    settings::Data data;

    Settings() {}

    ~Settings() {}

    void update_window_size(vec2 size) {
        data.window_size = size;
        //
        WindowResizeEvent* event = new WindowResizeEvent(
            static_cast<int>(size.x), static_cast<int>(size.y));
        App::get().processEvent(*event);
        delete event;
    }

    void update_master_volume(float nv) {
        data.masterVolume = nv;
        // std::cout << "master volume changed to " << data.masterVolume <<
        // std::endl;
        SetMasterVolume(data.masterVolume);
        // TODO support sound vs music volume
    }

    bool load_save_file() {
        std::ifstream ifs(Files::get().settings_filepath());
        if (!ifs.is_open()) {
            // std::cout << ("failed to find settings file (read)") <<
            // std::endl;
            return false;
        }

        std::cout << "reading settings file" << std::endl;
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        auto buf_str = buffer.str();

        bitsery::quickDeserialization<settings::InputAdapter>(
            {buf_str.begin(), buf_str.size()}, data);

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
