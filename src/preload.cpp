
#include "preload.h"

#include <istream>

int LOG_LEVEL = 2;

void Preload::load_config() {
    std::tuple<const char*, const char*>
        configFilePath = {strings::settings::CONFIG, "settings.json"};
        
    std::ifstream ifs(
        Files::get().fetch_resource_path(std::get<0>(configFilePath),
                                         std::get<1>(configFilePath)));
    if (ifs.good()) {
        const auto configJSON = nlohmann::json::parse(ifs);
        LOG_LEVEL = configJSON.value("LOG_LEVEL", 2);
        std::cout << "LOG_LEVEL read from file: " << LOG_LEVEL << std::endl;
    }else{
        std::cerr << "load_config error: configFilePath not found, "
                        "streamer couldn't open file. Check path: "
                    << Files::get().fetch_resource_path(std::get<0>(configFilePath),
                                                        std::get<1>(configFilePath))
                    << std::endl;
    }
}
