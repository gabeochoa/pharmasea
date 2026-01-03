

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
using OutArchive = zpp::bits::out<Buffer>;
using InArchive = zpp::bits::in<const Buffer>;

// TODO how do we support different games having different save file data
// requirements?

constexpr int MAX_LANG_LENGTH = 25;

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
    bool enable_lighting = true;
    bool snapCameraTo90 = false;
    bool isFullscreen = false;
    bool vsync_enabled = true;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                //
            self.engineVersion,        //
            self.master_volume,        //
            self.music_volume,         //
            self.sound_volume,         //
            self.show_streamer_safe_box, //
            self.snapCameraTo90,       //
            self.enable_postprocessing, //
            self.enable_lighting,      //
            self.isFullscreen,         //
            self.vsync_enabled,        //
            self.username,             //
            self.last_ip_joined,       //
            self.lang_name,            //
            self.ui_theme,             //
            self.resolution            //
        );
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
        os << "lighting: " << data.enable_lighting << std::endl;
        os << "last ip joined: " << data.last_ip_joined << std::endl;
        os << "lang name: " << data.lang_name << std::endl;
        os << "ui_theme: " << data.ui_theme << std::endl;
        os << "should snap camera: " << data.snapCameraTo90 << std::endl;
        os << "vsync enabled: " << data.vsync_enabled << std::endl;
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
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(  //
            self.name,   //
            self.filename  //
        );
    }
};

}  // namespace settings

struct Settings {
    static bool created;
    static Settings instance;
    static void create();
    [[nodiscard]] static Settings& get();

    settings::Data data;

    std::vector<settings::LanguageInfo> lang_options;

    Settings() : data(settings::Data()) {}

    ~Settings() {}

    void reset_to_default() {
        data = settings::Data();
        refresh_settings();
    }

    void update_resolution_from_index(int index);
    void update_window_size(rez::ResolutionInfo rez);
    void update_fullscreen(bool fs_enabled);
    void toggle_fullscreen();
    void update_last_used_ip_address(const std::string& ip);
    void update_master_volume(float nv);
    void update_music_volume(float nv);
    void update_sound_volume(float nv);
    void update_streamer_safe_box(bool sssb);
    void update_post_processing_enabled(bool pp_enabled);
    void update_lighting_enabled(bool lighting_enabled);
    void update_vsync_enabled(bool vsync_enabled);
    [[nodiscard]] int get_current_resolution_index() const;
    [[nodiscard]] std::vector<std::string> resolution_options() const;
    [[nodiscard]] std::string last_used_ip() const;
    bool load_save_file();
    bool write_save_file();
    void load_language_options();
    [[nodiscard]] std::vector<std::string> language_options();

    [[nodiscard]] int get_current_language_index();

    void update_language_from_index(int index);

    void update_language_name(const std::string& l);
    void update_ui_theme(const std::string& s);
    std::vector<std::string> get_ui_theme_options();
    int get_ui_theme_selected_index();
    void update_theme_from_index(int index);

    void refresh_settings();
};
