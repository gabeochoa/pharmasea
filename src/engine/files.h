
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
    [[nodiscard]] fs::path game_folder() const;
    bool ensure_game_folder_exists();
    [[nodiscard]] fs::path settings_filepath() const;
    [[nodiscard]] fs::path resource_folder() const;
    [[nodiscard]] fs::path game_controller_db() const;
    [[nodiscard]] std::string fetch_resource_path(std::string_view group,
                                                  std::string_view name) const;
    // TODO add a full cleanup to write folders in case we need to reset

    void folder_locations() const;
};
