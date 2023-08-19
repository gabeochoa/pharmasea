
#include "preload.h"

#include <istream>

#include "dataclass/ingredient.h"
#include "engine/ui_theme.h"
#include "recipe_library.h"

int LOG_LEVEL = 2;
std::vector<std::string> EXAMPLE_MAP;

namespace ui {
UITheme DEFAULT_THEME = UITheme();
}

auto Preload::load_json_config_file(
    const char* filename,
    const std::function<void(nlohmann::json)>& processor) {
    std::tuple<const char*, const char*> configFilePath = {
        strings::settings::CONFIG, filename};

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
        const auto configJSON = nlohmann::json::parse(
            ifs, nullptr /*parser_callback_t*/, true /*allow_exceptions=*/,
            true /* ignore comments */);

        processor(configJSON);

    } catch (const std::exception& e) {
        log_error("Preload::load_config: {} formatted improperly. {}", filename,
                  e.what());
    }
}

void Preload::load_config() {
    const auto _to_color =
        [](const nlohmann::basic_json<>::value_type value_type) -> Color {
        return Color{
            value_type[0].get<unsigned char>(),
            value_type[1].get<unsigned char>(),
            value_type[2].get<unsigned char>(),
            value_type[3].get<unsigned char>(),
        };
    };

    load_json_config_file(
        "settings.json", [_to_color](const nlohmann::json& contents) {
            LOG_LEVEL = contents.value("LOG_LEVEL", 2);
            std::cout << "LOG_LEVEL read from file: " << LOG_LEVEL << std::endl;

            EXAMPLE_MAP = contents["DEFAULT_MAP"];
            std::cout << "DEFAULT_MAP read from file: " << EXAMPLE_MAP.size()
                      << std::endl;

            const auto theme_name = contents["theme"];
            const auto themes = contents["themes"];

            const auto theme_data = themes[theme_name];

            ui::DEFAULT_THEME = ui::UITheme(_to_color(theme_data["font"]),  //
                                            _to_color(theme_data["darkfont"]),
                                            _to_color(theme_data["background"]),
                                            _to_color(theme_data["primary"]),
                                            _to_color(theme_data["secondary"]),
                                            _to_color(theme_data["accent"]),
                                            _to_color(theme_data["error"]));
        });
}

void Preload::load_models() {
    if (!ENABLE_MODELS)
    // TODO log
    {
        std::cout << "Skipping Model Loading" << std::endl;
        return;
    }

    load_json_config_file("models.json", [](const nlohmann::json& contents) {
        auto models = contents["models"];
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
    });

    log_info("Loaded model json successfully, {} models",
             ModelLibrary::get().size());
}

void Preload::load_drink_recipes() {
    load_json_config_file("drinks.json", [](const nlohmann::json& contents) {
        auto models = contents["drinks"];
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

            IngredientBitSet prereqs;
            prereqs.reset();
            if (object.contains("prereqs")) {
                for (auto ing_name : object["prereqs"]) {
                    auto ing_str = ing_name.get<std::string>();
                    std::optional<Ingredient> ing_opt =
                        magic_enum::enum_cast<Ingredient>(ing_str);
                    if (!ing_opt.has_value()) {
                        log_warn("failed to load prereq {} for {}", ing_str,
                                 base_name);
                        continue;
                    }
                    prereqs |= IngredientBitSet().set(ing_opt.value());
                }
            }

            RecipeLibrary::get().load(
                {
                    .drink = drink,
                    .base_name = base_name,
                    .viewer_name = object["viewer_name"].get<std::string>(),
                    .icon_name = object["icon_name"].get<std::string>(),
                    .ingredients = ingredients,
                    .prereqs = prereqs,
                },
                "INVALID", base_name.c_str());

            log_trace("loaded recipe {} ", base_name);
        }
    });

    log_info("Loaded drink json successfully, {} drinks",
             RecipeLibrary::get().size());
}

void Preload::load_textures() {
    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "drinks",
        [](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "external",
        [](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

    // Now load the one off ones

    load_json_config_file("textures.json", [](const nlohmann::json& contents) {
        auto textures = contents["textures"];

        for (auto object : textures) {
            auto folder = object["folder"].get<std::string>();
            auto filename = object["filename"].get<std::string>();
            auto library_name = object["library_name"].get<std::string>();

            TextureLibrary::get().load(
                Files::get().fetch_resource_path(folder, filename).c_str(),
                library_name.c_str());

            log_trace("loaded texture {} ", library_name);
        }

        log_info("Loaded texture json successfully, {} textures",
                 TextureLibrary::get().size());
    });
}
