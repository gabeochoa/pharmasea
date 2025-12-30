
#include "settings.h"

#include "../preload.h"

Settings Settings::instance;
bool Settings::created = false;

void Settings::create() {
    if (created) {
        log_error("Trying to create settings twice");
    }
    new (&instance) Settings();
    created = true;
}

Settings& Settings::get() { return instance; }

void Settings::update_language_from_index(int index) {
    // TODO handle exception
    auto li = lang_options[index];

    log_info("Loading translations for {} from {}", li.name, li.filename);

    Preload::get().on_language_change(li.name.c_str(), li.filename.c_str());

    // write to our data so itll be in the save file
    data.lang_name = li.name;
}

void Settings::update_ui_theme(const std::string& s) {
    // Only log if its not loading the default theme
    if (!s.empty()) log_info("updating UI theme to {}", s);

    Preload::get().update_ui_theme(s);
    data.ui_theme = s;
}

void Settings::update_theme_from_index(int index) {
    update_ui_theme(get_ui_theme_options()[index]);
}

std::vector<std::string> Settings::get_ui_theme_options() {
    return Preload::get().ui_theme_options();
}

int Settings::get_ui_theme_selected_index() {
    const auto options = get_ui_theme_options();

    auto it = std::find(options.begin(), options.end(), data.ui_theme);
    return (it != options.end())
               ? static_cast<int>(std::distance(options.begin(), it))
               : 0;  // default to 0
}

void Settings::update_resolution_from_index(int index) {
    update_window_size(rez::ResolutionExplorer::get().fetch(index));
}

void Settings::update_window_size(rez::ResolutionInfo rez) {
    data.resolution = rez;

    data.resolution.width = static_cast<int>(
        fminf(3860.f, fmaxf((float) data.resolution.width, 1280.f)));

    data.resolution.height = static_cast<int>(
        fminf(2160.f, fmaxf((float) data.resolution.height, 720.f)));

    //
    WindowResizeEvent* event =
        new WindowResizeEvent(data.resolution.width, data.resolution.height);

    __WIN_W = data.resolution.width;
    __WIN_H = data.resolution.height;

    App::get().processEvent(*event);
    delete event;
}

void Settings::update_fullscreen(bool fs_enabled) {
    data.isFullscreen = fs_enabled;
    WindowFullscreenEvent* event = new WindowFullscreenEvent(fs_enabled);
    App::get().processEvent(*event);
}

void Settings::toggle_fullscreen() { update_fullscreen(!data.isFullscreen); }

void Settings::update_last_used_ip_address(const std::string& ip) {
    data.last_ip_joined = ip;
}

void Settings::update_master_volume(float nv) {
    data.master_volume = util::clamp(nv, 0.f, 1.f);
    log_trace("master volume changed to {}", data.master_volume);

    MusicLibrary::get().update_volume(data.music_volume);
    SoundLibrary::get().update_volume(data.sound_volume);

    raylib::SetMasterVolume(data.master_volume);
}

void Settings::update_music_volume(float nv) {
    data.music_volume = util::clamp(nv, 0.f, 1.f);
    MusicLibrary::get().update_volume(data.music_volume);
}

void Settings::update_sound_volume(float nv) {
    data.sound_volume = util::clamp(nv, 0.f, 1.f);
    SoundLibrary::get().update_volume(data.sound_volume);
}

void Settings::update_streamer_safe_box(bool sssb) {
    data.show_streamer_safe_box = sssb;
}

void Settings::update_post_processing_enabled(bool pp_enabled) {
    data.enable_postprocessing = pp_enabled;
}

void Settings::update_lighting_enabled(bool lighting_enabled) {
    data.enable_lighting = lighting_enabled;
}

void Settings::update_vsync_enabled(bool vsync_enabled) {
    data.vsync_enabled = vsync_enabled;
    // Apply the setting immediately
    if (vsync_enabled) {
        raylib::SetWindowState(raylib::FLAG_VSYNC_HINT);
    } else {
        raylib::ClearWindowState(raylib::FLAG_VSYNC_HINT);
    }
}

[[nodiscard]] int Settings::get_current_resolution_index() const {
    int index = rez::ResolutionExplorer::get().index(data.resolution);
    // If we somehow got a resolution enabled that isnt in our valid
    // resolutions its likely because we went into fullscreen and the
    // monitor was different
    // TODO windows
    // This is most likely on windows since we cant read the possible
    // monitor settings yet, once we add that code we should be good
    if (index == -1) {
        rez::ResolutionExplorer::get().add(data.resolution);
        index = rez::ResolutionExplorer::get().index(data.resolution);
    }
    return index;
}

[[nodiscard]] std::vector<std::string> Settings::resolution_options() const {
    return rez::ResolutionExplorer::get().fetch_options();
}

[[nodiscard]] std::string Settings::last_used_ip() const {
    return data.last_ip_joined;
}

// TODO these could be private and inside the ctor/dtor with RAII if we are
// okay with running on get() and ignoring the result

// TODO music volumes only seem to take effect once you open SettingsLayer
bool Settings::load_save_file() {
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

    // Settings serialization is not version-tolerant yet; if the schema changes
    // (e.g. adding a new field), older files may fail to deserialize.
    // In that case, fall back to defaults instead of leaving partially-read data.
    settings::Data tmp{};
    const auto [err, ok] = bitsery::quickDeserialization<settings::InputAdapter>(
        {buf_str.begin(), buf_str.size()}, tmp);
    if (err != bitsery::ReaderError::NoError || !ok) {
        log_warn("Settings deserialize failed (err={}, ok={}); resetting to defaults",
                 (int) err, ok);
        data = settings::Data();
    } else {
        data = tmp;
    }

    refresh_settings();

    log_info("Settings Loaded: {}", data);
    log_trace("End loading settings file");
    ifs.close();
    return true;
}

// TODO instead of writing to a string and then file
// theres a way to write directly to the file
// https://github.com/fraillt/bitsery/blob/master/examples/file_stream.cpp
bool Settings::write_save_file() {
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

void Settings::load_language_options() {
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

[[nodiscard]] std::vector<std::string> Settings::language_options() {
    std::vector<std::string> options;
    options.reserve(lang_options.size());
    for (const auto& li : lang_options) {
        options.push_back(li.name);
    }
    return options;
}

[[nodiscard]] int Settings::get_current_language_index() {
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

void Settings::update_language_name(const std::string& l) {
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

// Note: Basically once we load the file,
// we run into an issue where our settings is correct,
// but the underlying data isnt being used
//
// This function is used by the load to kick raylib into
// the right config
void Settings::refresh_settings() {
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
    update_vsync_enabled(data.vsync_enabled);
}
