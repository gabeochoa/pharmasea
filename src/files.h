
#pragma once

#include "external_include.h"
#include "singleton.h"

namespace fs = std::filesystem;

const std::string GAME_FOLDER = "pharmasea";
const std::string SETTINGS_FILE_NAME = "settings.bin";

SINGLETON_FWD(Files)
struct Files {
    SINGLETON(Files)
    Files();
    fs::path game_folder();
    bool ensure_game_folder_exists();
    fs::path settings_filepath();
    fs::path resource_folder();
    std::string fetch_resource_path(std::string group, std::string name);
    // TODO add a full cleanup to write folders in case we need to reset

    void folder_locations();
};
