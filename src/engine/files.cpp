#include "files.h"

#include "globals.h"
#include "log.h"

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include <sago/platform_folders.h>

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

std::shared_ptr<Files> Files_single;

Files::Files(const FilesConfig& config)
    : root(config.root_folder), settings_file(config.settings_file_name) {
    ensure_game_folder_exists();
}

fs::path Files::game_folder() const {
    const fs::path master_folder(sago::getSaveGamesFolder1());
    return master_folder / fs::path(root);
}

bool Files::ensure_game_folder_exists() {
    fs::path fld = game_folder();
    if (fs::exists(fld)) {
        return true;
    }
    bool created = fs::create_directory(fld);
    if (created) {
        log_info("Created Game Folder: {}", fld);
        return true;
    }
    return false;
}

fs::path Files::settings_filepath() const {
    fs::path file(settings_file);
    fs::path full_path = game_folder() / file;
    return full_path;
}

fs::path Files::resource_folder() const { return fs::path("./resources"); }

fs::path Files::game_controller_db() const {
    return (resource_folder() / fs::path("gamecontrollerdb.txt")).string();
}

std::string Files::fetch_resource_path(std::string_view group,
                                       std::string_view name) const {
    return (resource_folder() / fs::path(group) / fs::path(name)).string();
}

void Files::for_resources_in_group(
    std::string_view group,
    std::function<void(std::string, std::string, std::string)> cb) const {
    auto folder_path = (resource_folder() / fs::path(group));

    try {
        auto dir_iter = std::filesystem::directory_iterator{folder_path};
        for (auto const& dir_entry : dir_iter) {
            cb(dir_entry.path().stem().string(), dir_entry.path().string(),
               dir_entry.path().extension().string());
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return;
    }
}

void Files::for_resources_in_folder(
    std::string_view group, std::string_view folder,
    std::function<void(std::string, std::string)> cb) const {
    auto folder_path = (resource_folder() / fs::path(group) / fs::path(folder));

    try {
        auto dir_iter = std::filesystem::directory_iterator{folder_path};
        for (auto const& dir_entry : dir_iter) {
            cb(dir_entry.path().stem().string(), dir_entry.path().string());
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return;
    }
}

void Files::folder_locations() const {
    using namespace std;
    using namespace sago;
    log_info("Config: ", getConfigHome());
    log_info("Data: ", getDataHome());
    log_info("State: ", getStateDir());
    log_info("Cache: ", getCacheDir());
    log_info("Documents: ", getDocumentsFolder());
    log_info("Desktop: ", getDesktopFolder());
    log_info("Pictures: ", getPicturesFolder());
    log_info("Public: ", getPublicFolder());
    log_info("Music: ", getMusicFolder());
    log_info("Video: ", getVideoFolder());
    log_info("Download: ", getDownloadFolder());
    log_info("Save Games 1: ", getSaveGamesFolder1());
    log_info("Save Games 2: ", getSaveGamesFolder2());
    vector<string> extraData;
    appendAdditionalDataDirectories(extraData);
    for (size_t i = 0; i < extraData.size(); ++i) {
        log_info("Additional data {}: {}", i, extraData.at(i));
    }
}
