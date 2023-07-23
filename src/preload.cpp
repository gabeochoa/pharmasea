
#include "preload.h"

#include <istream>

int LOG_LEVEL = 2;
std::vector<std::string> EXAMPLE_MAP;

void load_settings_config() {
    std::tuple<const char*, const char*> configFilePath = {
        strings::settings::CONFIG, "settings.json"};

    std::ifstream ifs(Files::get().fetch_resource_path(
        std::get<0>(configFilePath), std::get<1>(configFilePath)));

    if (!ifs.good()) {
        std::cerr << "load_config error: configFilePath not found, "
                     "streamer couldn't open file. Check path: "
                  << Files::get().fetch_resource_path(
                         std::get<0>(configFilePath),
                         std::get<1>(configFilePath))
                  << std::endl;
        return;
    }

    try {
        const auto configJSON = nlohmann::json::parse(ifs);

        LOG_LEVEL = configJSON.value("LOG_LEVEL", 2);
        std::cout << "LOG_LEVEL read from file: " << LOG_LEVEL << std::endl;

        EXAMPLE_MAP = configJSON["DEFAULT_MAP"];
        std::cout << "DEFAULT_MAP read from file: " << EXAMPLE_MAP.size()
                  << std::endl;

    } catch (const std::exception& e) {
        std::cerr
            << "Preload::load_config: settings.json formatted improperly. "
            << e.what() << std::endl;
    }
}

void load_model_configs() {
    std::tuple<const char*, const char*> configFilePath = {
        strings::settings::CONFIG, "models.json"};

    std::ifstream ifs(Files::get().fetch_resource_path(
        std::get<0>(configFilePath), std::get<1>(configFilePath)));

    if (!ifs.good()) {
        std::cerr << "load_config error: configFilePath not found, "
                     "streamer couldn't open file. Check path: "
                  << Files::get().fetch_resource_path(
                         std::get<0>(configFilePath),
                         std::get<1>(configFilePath))
                  << std::endl;
        return;
    }

    try {
        const auto configJSON = nlohmann::json::parse(ifs);

        auto models = configJSON["models"];

        for (auto object : models) {
            ModelInfoLibrary::ModelLoadingInfo modelInfo;
            modelInfo.folder = object["folder"].get<std::string>();
            modelInfo.filename = object["filename"].get<std::string>();
            modelInfo.library_name = object["library_name"].get<std::string>();
            modelInfo.size_scale = object["size_scale"].get<float>();
            modelInfo.position_offset.x =
                object["position_offset"][0].get<float>();
            modelInfo.position_offset.y =
                object["position_offset"][1].get<float>();
            modelInfo.position_offset.z =
                object["position_offset"][2].get<float>();
            modelInfo.rotation_angle = object["rotation_angle"].get<float>();

            // Load the ModelLoadingInfo into the ModelInfoLibrary
            ModelInfoLibrary::get().load({
                .folder = modelInfo.folder,
                .filename = modelInfo.filename,
                .library_name = modelInfo.library_name,
                .size_scale = modelInfo.size_scale,
                .position_offset = modelInfo.position_offset,
                .rotation_angle = modelInfo.rotation_angle,
            });
        }

    } catch (const std::exception& e) {
        std::cerr << "Preload::load_config: models.json formatted improperly. "
                  << e.what() << std::endl;
    }
}

void Preload::load_config() {
    load_settings_config();
    load_model_configs();
}
