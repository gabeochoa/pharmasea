
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
#ifdef WRITE_FILES
        const fs::path master_folder(sago::getSaveGamesFolder1());
#else 
        const fs::path master_folder("./");
#endif 
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

    fs::path resource_folder() { return fs::path("./resources"); }

    std::string fetch_resource_path(std::string group, std::string name) {
        return (resource_folder() / fs::path(group) / fs::path(name)).string();
    }

    // TODO add a full cleanup to write folders in case we need to reset

#ifdef WRITE_FILES
    void folder_locations() {
        using namespace std;
        using namespace sago;
        cout << "Config: " << getConfigHome() << "\n";
        cout << "Data: " << getDataHome() << "\n";
        cout << "State: " << getStateDir() << "\n";
        cout << "Cache: " << getCacheDir() << "\n";
        cout << "Documents: " << getDocumentsFolder() << "\n";
        cout << "Desktop: " << getDesktopFolder() << "\n";
        cout << "Pictures: " << getPicturesFolder() << "\n";
        cout << "Public: " << getPublicFolder() << "\n";
        cout << "Music: " << getMusicFolder() << "\n";
        cout << "Video: " << getVideoFolder() << "\n";
        cout << "Download: " << getDownloadFolder() << "\n";
        cout << "Save Games 1: " << getSaveGamesFolder1() << "\n";
        cout << "Save Games 2: " << getSaveGamesFolder2() << "\n";
        vector<string> extraData;
        appendAdditionalDataDirectories(extraData);
        for (size_t i = 0; i < extraData.size(); ++i) {
            cout << "Additional data " << i << ": " << extraData.at(i) << "\n";
        }
    }
#endif 
};
