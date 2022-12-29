
#pragma once

#include <filesystem>
#include <string>

#include "singleton.h"

namespace fs = std::filesystem;

struct FilesConfig {
    std::string_view root_folder;
    std::string_view settings_file_name;
};

SINGLETON_FWD(Files)
struct Files {
    SINGLETON_PARAM(Files, FilesConfig)

    std::string root;
    std::string settings_file;

    explicit Files(FilesConfig config);
    fs::path game_folder();
    bool ensure_game_folder_exists();
    fs::path settings_filepath();
    fs::path resource_folder();
    std::string fetch_resource_path(std::string group, std::string name);
    // TODO add a full cleanup to write folders in case we need to reset

    void folder_locations();
};
