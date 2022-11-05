
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
    float master_volume = 0.5f;
    bool show_streamer_safe_box = false;
    std::string username = "";

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(version);
        s.object(window_size);
        s.value4b(master_volume);
        s.value1b(show_streamer_safe_box);
        s.text1b(username, network::MAX_NAME_LENGTH);
    }
};

std::ostream& operator<<(std::ostream& os, const Data& data) {
    os << "Settings(" << std::endl;
    os << "version: " << data.version << std::endl;
    os << "resolution: " << data.window_size.x << ", " << data.window_size.y
       << std::endl;
    os << "master vol: " << data.master_volume << std::endl;
    os << "Safe box: " << data.show_streamer_safe_box << std::endl;
    os << "username: " << data.username << std::endl;
    os << ")" << std::endl;
    return os;
}

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
        update_window_size(data.window_size);
        update_master_volume(data.master_volume);
        // streamer box doesnt need update
    }

    void update_window_size(vec2 size) {
        if (size.x == 0 || size.y == 0) {
            size = {800, 600};
        }

        data.window_size = size;
        //
        WindowResizeEvent* event = new WindowResizeEvent(
            static_cast<int>(size.x), static_cast<int>(size.y));
        App::get().processEvent(*event);
        delete event;
    }

    void update_master_volume(float nv) {
        data.master_volume = nv;
        // std::cout << "master volume changed to " << data.master_volume
        // << std::endl;
        SetMasterVolume(data.master_volume);
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
