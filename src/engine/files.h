
#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "singleton.h"

namespace fs = std::filesystem;

struct FilesConfig {
    std::string_view root_folder;
    std::string_view settings_file_name;
};

struct Files {
    std::string root = "example_game";
    std::string settings_file = "settings.bin";

    explicit Files(const FilesConfig& config);
    [[nodiscard]] fs::path game_folder() const;
    bool ensure_game_folder_exists();
    [[nodiscard]] fs::path settings_filepath() const;
    [[nodiscard]] fs::path resource_folder() const;
    [[nodiscard]] fs::path game_controller_db() const;
    [[nodiscard]] std::string fetch_resource_path(std::string_view group,
                                                  std::string_view name) const;
    void for_resources_in_group(
        std::string_view group,
        // TODO replace with struct?
        const std::function<void(std::string, std::string, std::string)>&)
        const;
    void for_resources_in_folder(
        std::string_view group, std::string_view folder,
        const std::function<void(std::string, std::string)>&) const;

    // TODO add a full cleanup to write folders in case we need to reset

    void folder_locations() const;

    static void create(const FilesConfig&);
    static Files& get();

   private:
    Files() {}
    static bool created;
    static Files instance;
    Files(const Files&);
    Files& operator=(const Files&);
};
