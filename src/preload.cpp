
#include "preload.h"

#include <istream>

#include "dataclass/ingredient.h"
#include "recipe_library.h"

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
    if (!ENABLE_MODELS)
    // TODO log
    {
        std::cout << "Skipping Model Loading" << std::endl;
        return;
    }

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
            ModelLibrary::get().load({
                .folder = modelInfo.folder.c_str(),
                .filename = modelInfo.filename.c_str(),
                .libraryname = modelInfo.library_name.c_str(),
            });
            log_trace("loaded {} as {} ", modelInfo.filename,
                      modelInfo.library_name);
        }

        // TODO move to log
        // TODO add count of models
        std::cout << "Loaded model json successfully" << std::endl;
    } catch (const std::exception& e) {
        log_warn("Preload::load_config: models.json formatted improperly. {}",
                 e.what());
    }
}

void load_drink_recipes() {
    std::tuple<const char*, const char*> configFilePath = {
        strings::settings::CONFIG, "drinks.json"};

    std::ifstream ifs(Files::get().fetch_resource_path(
        std::get<0>(configFilePath), std::get<1>(configFilePath)));

    if (!ifs.good()) {
        // TODO move to log
        std::cerr << "load_config error: configFilePath not found, "
                     "streamer couldn't open file. Check path: "
                  << Files::get().fetch_resource_path(
                         std::get<0>(configFilePath),
                         std::get<1>(configFilePath))
                  << std::endl;
        return;
    }

    try {
        const auto configJSON = nlohmann::json::parse(
            ifs, nullptr /*parser_callback_t*/, true /*allow_exceptions=*/,
            true /* ignore comments */);

        auto models = configJSON["drinks"];

        for (auto object : models) {
            auto base_name = object["name"].get<std::string>();
            IngredientBitSet ingredients;

            for (auto ing_name : object["ingredients"]) {
                auto ing_str = ing_name.get<std::string>();
                std::optional<Ingredient> ing_opt =
                    magic_enum::enum_cast<Ingredient>(ing_str);
                if (!ing_opt.has_value()) {
                    log_warn("failed to load ingredient {} for {}", ing_str,
                             base_name);
                    continue;
                }
                ingredients |= IngredientBitSet().set(ing_opt.value());
            }

            Drink drink;
            {
                std::optional<Drink> drink_opt =
                    magic_enum::enum_cast<Drink>(base_name);
                if (!drink_opt.has_value()) {
                    log_warn("failed to load Drink {} for {}", base_name,
                             base_name);
                    continue;
                }
                drink = drink_opt.value();
            }

            RecipeLibrary::get().load(
                {
                    .drink = drink,
                    .base_name = base_name,
                    .model_name = object["model_name"].get<std::string>(),
                    .viewer_name = object["viewer_name"].get<std::string>(),
                    .icon_name = object["icon_name"].get<std::string>(),
                    .ingredients = ingredients,
                },
                "INVALID", base_name.c_str());

            log_info("loaded recipe {} ", base_name);
        }

        log_info("Loaded drink recipe json successfully");
    } catch (const std::exception& e) {
        // TODO extract this logic for the three functions into helper
        log_warn("Preload::load_config: drinks.json formatted improperly. {}",
                 e.what());
    }
}

void Preload::load_config() {
    load_settings_config();
    load_model_configs();
    load_drink_recipes();
}
