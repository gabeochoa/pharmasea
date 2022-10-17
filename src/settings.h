
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

SINGLETON_FWD(Settings)
struct Settings {
    SINGLETON(Settings)

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
    } data;

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
        // TODO sent out audio volume change event
    }

    bool load_save_file() {
        std::ifstream ifs(Files::get().settings_filepath());
        if (!ifs.is_open()) {
            // std::cout << ("failed to find settings file (read)") <<
            // std::endl;
            return false;
        }

        std::cout << "reading settings file" << std::endl;
        std::string line;
        while (getline(ifs, line)) {
            std::cout << line << std::endl;
            auto tokens = util::split_string(line, ",");
            if (tokens[0] == "version") {
                data.version = atoi(tokens[1].c_str());
            } else if (tokens[0] == "window_size") {
                float x = static_cast<float>(atof(tokens[1].c_str()));
                float y = static_cast<float>(atof(tokens[2].c_str()));
                update_window_size({x, y});
            } else if (tokens[0] == "master_volume") {
                update_master_volume(static_cast<float>(atof(tokens[1].c_str())));
            } else {
                // TODO handle unknown data
            }
        }
        std::cout << "end settings file" << std::endl;
        ifs.close();
        return true;
    }

    bool write_save_file() {
        std::ofstream ofs(Files::get().settings_filepath());
        if (!ofs.is_open()) {
            std::cout << ("failed to find settings file (write)") << std::endl;
            return false;
        }

        std::string line;
        ofs << "version," << data.version << std::endl;
        ofs << "window_size," << data.window_size.x << "," << data.window_size.y
            << std::endl;
        ofs << "master_volume," << data.masterVolume << std::endl;

        ofs.close();
        return true;
    }
};
