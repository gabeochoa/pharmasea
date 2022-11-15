#include "files.h"

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

Files::Files() { ensure_game_folder_exists(); }

fs::path Files::game_folder() {
    const fs::path master_folder(sago::getSaveGamesFolder1());
    return master_folder / fs::path(GAME_FOLDER);
}

bool Files::ensure_game_folder_exists() {
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

fs::path Files::settings_filepath() {
    fs::path file(SETTINGS_FILE_NAME);
    fs::path full_path = game_folder() / file;
    return full_path;
}

fs::path Files::resource_folder() { return fs::path("./resources"); }

std::string Files::fetch_resource_path(std::string group, std::string name) {
    return (resource_folder() / fs::path(group) / fs::path(name)).string();
}

// TODO add a full cleanup to write folders in case we need to reset

void Files::folder_locations() {
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
