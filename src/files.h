
#pragma once

#include "external_include.h"
#include "singleton.h"

namespace fs = std::filesystem;

const std::string GAME_FOLDER = "pharmasea";
const std::string SETTINGS_FILE_NAME = "settings.bin";

SINGLETON_FWD(Files)
struct Files {
    SINGLETON(Files)

    Files() { ensure_game_folder_exists(); }

    fs::path game_folder() {
        const fs::path master_folder(sago::getSaveGamesFolder1());
        return master_folder / fs::path(GAME_FOLDER);
    }

    bool ensure_game_folder_exists() {
        fs::path fld = game_folder();
        if (fs::exists(fld)) {
            return true;
        }
        bool created = fs::create_directory(fld);
        if (created) {
            std::cout << "Created Game Folder: " << fld << std::endl;
            return true;
        }
        return false;
    }

    fs::path settings_filepath() {
        fs::path file(SETTINGS_FILE_NAME);
        fs::path full_path = game_folder() / file;
        return full_path;
    }
    
    // TODO add a full cleanup to write folders in case we need to reset

    void folder_locations() {
        std::cout << "Config: " << sago::getConfigHome() << "\n";
        std::cout << "Data: " << sago::getDataHome() << "\n";
        std::cout << "State: " << sago::getStateDir() << "\n";
        std::cout << "Cache: " << sago::getCacheDir() << "\n";
        std::cout << "Documents: " << sago::getDocumentsFolder() << "\n";
        std::cout << "Desktop: " << sago::getDesktopFolder() << "\n";
        std::cout << "Pictures: " << sago::getPicturesFolder() << "\n";
        std::cout << "Public: " << sago::getPublicFolder() << "\n";
        std::cout << "Music: " << sago::getMusicFolder() << "\n";
        std::cout << "Video: " << sago::getVideoFolder() << "\n";
        std::cout << "Download: " << sago::getDownloadFolder() << "\n";
        std::cout << "Save Games 1: " << sago::getSaveGamesFolder1() << "\n";
        std::cout << "Save Games 2: " << sago::getSaveGamesFolder2() << "\n";
        std::vector<std::string> extraData;
        sago::appendAdditionalDataDirectories(extraData);
        for (size_t i = 0; i < extraData.size(); ++i) {
            std::cout << "Additional data " << i << ": " << extraData.at(i)
                      << "\n";
        }
    }
};
