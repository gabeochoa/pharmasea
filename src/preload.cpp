
#include "preload.h"

#include <istream>

int LOG_LEVEL = 2;
std::vector<std::string> EXAMPLE_MAP;

void Preload::load_config() {
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
