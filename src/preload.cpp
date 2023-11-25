
#include "preload.h"

#include <istream>

#include "config_key_library.h"
#include "dataclass/ingredient.h"
#include "dataclass/settings.h"
#include "engine/font_library.h"
#include "engine/keymap.h"
#include "engine/ui/theme.h"
#include "magic_enum/magic_enum.hpp"
#include "map_generation.h"
#include "recipe_library.h"
#include "upgrade_library.h"

// dataclass/settings.h
std::vector<UpgradeType> upgrade_rounds;

float DEADZONE = 0.25f;
int LOG_LEVEL = 2;
std::vector<std::string> EXAMPLE_MAP;
i18n::LocalizationText* localization;

namespace wfc {
MapGenerationInformation MAP_GEN_INFO;
vec3 MAX_SPOT;
Rectangle SPAWN_AREA;
Rectangle TRASH_AREA;
}  // namespace wfc

namespace ui {
UITheme UI_THEME = UITheme();
std::vector<std::string> theme_keys;
std::map<std::string, UITheme> themes;
}  // namespace ui

std::vector<std::string> Preload::ui_theme_options() { return ui::theme_keys; }

void Preload::update_ui_theme(const std::string& theme) {
    using namespace ui;
    if (themes.contains(theme)) {
        UI_THEME = themes.at(theme);
    }
}

void Preload::_load_font_from_name(const std::string& filename,
                                   const std::string& lang) {
    auto path =
        Files::get().fetch_resource_path(strings::settings::FONTS, filename);

    auto translations_path = Files::get().fetch_resource_path(
        // Note: we are using the po since we just want the strings
        strings::settings::TRANSLATIONS, fmt::format("{}.po", lang));
    std::ifstream ifs(translations_path);

    FontLoadingInfo fli{.filename = path.c_str(),
                        .size = 400,
                        .font_chars = nullptr,
                        .glyph_count = 0};

    // if it correctly loaded grab all the characters and load the font for
    // them
    if (
        // Ignore english because i never filled out the po file...
        lang != "en_us" &&
        // check to make sure the po file for the language loaded
        ifs.good()) {
        std::stringstream buffer;
        buffer << ifs.rdbuf();

        // TODO :BE: this is a copypaste from font_util.h
        int codepointCount = 0;
        int* codepoints =
            raylib::LoadCodepoints(buffer.str().c_str(), &codepointCount);

        int codepointNoDupsCounts = 0;
        int* codepointsNoDups = CodepointRemoveDuplicates(
            codepoints, codepointCount, &codepointNoDupsCounts);

        raylib::UnloadCodepoints(codepoints);

        fli.font_chars = codepointsNoDups;
        fli.glyph_count = codepointNoDupsCounts;
    } else {
        log_warn("failed to open translations po file {} for {}",
                 translations_path, lang);
    }

    FontLibrary::get().load(fli, lang.c_str());
}

const char* Preload::get_font_for_lang(const char* lang_name) {
    // TODO :BE: eventually load from json config
    switch (hashString(lang_name)) {
        case hashString("en_rev"):
        case hashString("en_us"):
            return "Gaegu-Bold.ttf";
        case hashString("ko_kr"):
            return "NotoSansKR.ttf";
    }
    return "Gaegu-Bold.ttf";
}

void Preload::load_fonts() {
    // Font loading must happen after InitWindow

    // TODO :BE: load from json
    std::array<const char*, 3> langs = {
        "en_us",
        "en_rev",
        "ko_kr",
    };

    for (const auto lang : langs) {
        _load_font_from_name(get_font_for_lang(lang), lang);
    }

    // default to en us
    font = FontLibrary::get().get("en_us");

    // font = load_karmina_regular();

    // NOTE if you go back to files, load fonts from install folder, instead
    // of local path
    //
    // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
}

void Preload::on_language_change(const char* lang_name, const char* fn) {
    // Reset localization and reload from file...
    if (localization) delete localization;
    localization = new i18n::LocalizationText(fn);

    // During startup we load settings before preload, but that means that
    // the fonts arent loaded yet.
    //
    // So skip loading the font since itll load later
    if (!FontLibrary::get().contains("en_us")) {
        if (completed_preload_once) {
            log_warn("Failed to find english font in the font library");
        }
        return;
    }

    if (!FontLibrary::get().contains(lang_name)) {
        log_warn("Couldnt find a font for {}, using en_us instead", lang_name);
        font = FontLibrary::get().get("en_us");
        return;
    }

    // Once reloaded load correct font for language
    font = FontLibrary::get().get(lang_name);
}

void Preload::write_json_config_file(const char* filename,
                                     const nlohmann::json& configJSON) {
    std::ofstream ofs(filename);
    if (!ofs.good()) {
        std::cerr << "write_json_config_file error: Couldn't open file "
                     "for writing: "
                  << filename << std::endl;
        return;
    }

    log_info("{}", filename);

    ofs << configJSON.dump(4);  // Write the JSON to the file with formatting
    ofs.close();
}

void Preload::write_keymap() {
    std::tuple<const char*, const char*> configFilePath = {
        strings::settings::CONFIG,
        strings::settings::KEYMAP_FILE,
    };
    auto filename = Files::get().fetch_resource_path(
        std::get<0>(configFilePath), std::get<1>(configFilePath));
    this->write_json_config_file(filename.c_str(),
                                 KeyMap::get().serializeFullMap());
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

    load_json_config_file("settings.json", [&](const nlohmann::json& contents) {
        LOG_LEVEL = contents.value("LOG_LEVEL", 2);
        log_trace("LOG_LEVEL read from file: {}", LOG_LEVEL);

        DEADZONE = contents.value("DEADZONE", 0.25f);

        EXAMPLE_MAP = contents["DEFAULT_MAP"];
        log_trace("DEFAULT_MAP read from file: {}", EXAMPLE_MAP.size());

        const auto& theme_name = contents.value("theme", "default");
        const auto& j_themes = contents["themes"];

        for (const auto& theme_obj : j_themes.get<nlohmann::json::object_t>()) {
            std::string key = theme_obj.first;
            auto theme_data = theme_obj.second;
            ui::themes[key] = ui::UITheme(_to_color(theme_data["font"]),  //
                                          _to_color(theme_data["darkfont"]),
                                          _to_color(theme_data["background"]),
                                          _to_color(theme_data["primary"]),
                                          _to_color(theme_data["secondary"]),
                                          _to_color(theme_data["accent"]),
                                          _to_color(theme_data["error"]));
            ui::theme_keys.push_back(key);
        }
        update_ui_theme(theme_name);
    });
}

void Preload::load_models() {
    if (!ENABLE_MODELS) {
        log_warn("Skipping Model Loading");
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
                {.drink = drink,
                 .base_name = base_name,
                 .viewer_name = object["viewer_name"].get<std::string>(),
                 .icon_name = object["icon_name"].get<std::string>(),
                 .ingredients = ingredients,
                 .prereqs = prereqs,
                 .num_drinks = object.value("num_drinks", 1),
                 .requires_upgrade = object.value("requires_upgrade", false)},
                "INVALID", base_name.c_str());

            log_trace("loaded recipe {} ", base_name);
        }
    });

    log_info("Loaded drink json successfully, {} drinks",
             RecipeLibrary::get().size());
}

void Preload::load_textures() {
    // TODO add a warning for when you are loading two images with the same name
    // because we dont distinguish between folders this is more likely than youd
    // think
    // TODO add support for prepending the folder name to the texture name

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

    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "upgrade",
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

void Preload::load_keymapping() {
    load_json_config_file(strings::settings::KEYMAP_FILE,
                          [](const nlohmann::json& contents) {
                              KeyMap::get().deserializeFullMap(contents);
                          });
}

void Preload::load_map_generation_info() {
    using namespace wfc;

    load_json_config_file(
        "map_generator_input.json", [](const nlohmann::json& contents) {
            MAP_GEN_INFO.rows = contents.value("max_rows", 3);
            MAP_GEN_INFO.cols = contents.value("max_cols", 3);

            MAX_SPOT = vec3{
                static_cast<float>(MAP_GEN_INFO.rows),
                0,
                static_cast<float>(MAP_GEN_INFO.cols),
            };

            // TODO so we draw from the center and this will look off
            SPAWN_AREA = Rectangle{MAX_SPOT.x - 1, MAX_SPOT.z - 12, 7, 2};
            TRASH_AREA = Rectangle{MAX_SPOT.x - 1, MAX_SPOT.z - 4, 7, 2};

            auto jpatterns = contents["patterns"];

            int id = 0;
            for (auto jpat : jpatterns) {
                Connections connections;
                for (const auto& c : jpat["connections"]) {
                    connections.insert(
                        magic_enum::enum_cast<wfc::Rose>(c.get<std::string>())
                            .value());
                }

                MAP_GEN_INFO.patterns.emplace_back(Pattern{
                    .id = id++,
                    .pat = jpat["pat"].get<std::vector<std::string>>(),
                    .connections = connections,
                    .required = jpat.value("required", false),
                    .any_connection = jpat.value("any_connection", false),
                    .max_count = jpat.value("max_count", -1),
                    .edge_only = jpat.value("edge_only", false),
                });
            }

            log_info("Loaded map generation details containing {} patterns",
                     MAP_GEN_INFO.patterns.size());
        });
}

void Preload::load_upgrades() {
    const auto str_to_entity_type = [](const std::string& str) {
        try {
            return magic_enum::enum_cast<EntityType>(
                       str, magic_enum::case_insensitive)
                .value();
        } catch (std::exception e) {
            std::cout << ("exception converting entity type: {}", e.what())
                      << std::endl;
        }
        return EntityType::Unknown;
    };

    const auto str_to_drink = [](const std::string& str) {
        try {
            return magic_enum::enum_cast<Drink>(str,
                                                magic_enum::case_insensitive)
                .value();
        } catch (std::exception e) {
            std::cout << ("exception converting drink type: {}", e.what())
                      << std::endl;
        }
        return Drink::coke;
    };

    const auto load_config_values = [str_to_entity_type, str_to_drink](
                                        const nlohmann::json& config_values) {
        for (auto config : config_values) {
            const auto name = config["name"].get<std::string>();
            ConfigKey key = to_configkey(name);

            const auto key_type = get_type(key);
            ConfigValueType value;

            const auto& efv = config["value"];
            switch (key_type) {
                case ConfigKeyType::Entity: {
                    value = str_to_entity_type(efv.get<std::string>());
                } break;
                case ConfigKeyType::Drink: {
                    value = str_to_drink(efv.get<std::string>());
                } break;
                case ConfigKeyType::Float:
                    value = efv.get<float>();
                    break;
                case ConfigKeyType::Bool:
                    value = efv.get<bool>();
                    break;
                case ConfigKeyType::Int:
                    value = efv.get<int>();
                    break;
            }

            ConfigValueLibrary::get().load(
                {
                    .key = key,
                    .value = value,
                },
                "INVALID",
                std::string(magic_enum::enum_name<ConfigKey>(key)).c_str());
        }
    };
    const auto load_upgrades = [&](const nlohmann::json& upgrades) {
        const auto parse_effect =
            [&](const nlohmann::json& effects) -> UpgradeEffect {
            const auto key = to_configkey(effects["name"].get<std::string>());
            const auto key_type = get_type(key);

            ConfigValueType value;

            const auto& efv = effects["value"];
            switch (key_type) {
                case ConfigKeyType::Entity: {
                    value = str_to_entity_type(efv.get<std::string>());
                } break;
                case ConfigKeyType::Drink: {
                    value = str_to_drink(efv.get<std::string>());
                } break;
                case ConfigKeyType::Float:
                    value = efv.get<float>();
                    break;
                case ConfigKeyType::Bool:
                    value = efv.get<bool>();
                    break;
                case ConfigKeyType::Int:
                    value = efv.get<int>();
                    break;
            }

            return UpgradeEffect{
                .name = key,
                .operation =
                    to_operation(effects["operation"].get<std::string>()),
                .value = value,
            };
        };

        const auto parse_effects =
            [&](const nlohmann::json& effects) -> std::vector<UpgradeEffect> {
            std::vector<UpgradeEffect> output;
            for (const auto& effect : effects) {
                output.push_back(parse_effect(effect));
            }
            return output;
        };

        const auto parse_prereq =
            [&](const nlohmann::json& req) -> UpgradeRequirement {
            const auto key = to_configkey(req["name"].get<std::string>());
            const auto key_type = get_type(key);

            ConfigValueType value;

            const auto& efv = req["value"];
            switch (key_type) {
                case ConfigKeyType::Entity: {
                    value = str_to_entity_type(efv.get<std::string>());
                } break;
                case ConfigKeyType::Drink: {
                    value = str_to_drink(efv.get<std::string>());
                } break;
                case ConfigKeyType::Float:
                    value = efv.get<float>();
                    break;
                case ConfigKeyType::Bool:
                    value = efv.get<bool>();
                    break;
                case ConfigKeyType::Int:
                    value = efv.get<int>();
                    break;
            }

            return UpgradeRequirement{
                .name = key,
                .value = value,
            };
        };

        const auto parse_prereqs =
            [&](const nlohmann::json& reqs) -> std::vector<UpgradeRequirement> {
            std::vector<UpgradeRequirement> output;
            for (const auto& req : reqs) {
                output.push_back(parse_prereq(req));
            }
            return output;
        };

        const auto parse_required_machines =
            [&](const std::string& name,
                const nlohmann::json& machines) -> std::vector<EntityType> {
            std::vector<EntityType> output;
            // log_info("machine {}", machines.size());

            for (const auto& machine : machines) {
                const std::string& etstr = machine.get<std::string>();
                // log_info("got {}", etstr);

                const auto et = magic_enum::enum_cast<EntityType>(
                    util::remove_underscores(etstr),
                    magic_enum::case_insensitive);

                if (!et.has_value()) {
                    log_error(
                        "{} has required machine {} but we could'nt find a "
                        "matching entity type ",
                        name, machine);
                    continue;
                }

                output.push_back(et.value());
            }
            return output;
        };

        const auto parse_active_hours =
            [&](const nlohmann::json& upgrade) -> UpgradeActiveHours {
            auto all_day = UpgradeActiveHours().set();

            auto active_hours = UpgradeActiveHours().reset();

            for (auto& dur_str :
                 upgrade.value("active_hours", std::vector<std::string>())) {
                size_t dash_pos = dur_str.find('-');
                if (dash_pos == std::string::npos) {
                    log_warn("Failed to parse active_hours {}", dur_str);
                    continue;
                }

                int start = std::stoi(dur_str.substr(0, dash_pos));
                int end = std::stoi(dur_str.substr(dash_pos + 1));

                for (int i = start; i <= end; i++) {
                    active_hours.set(i);
                }
            }

            return active_hours.any() ? active_hours : all_day;
        };

        for (auto upgrade : upgrades) {
            const auto name = upgrade["name"].get<std::string>();
            const auto disabled = upgrade.value("disabled", "");
            if (!disabled.empty()) {
                log_warn("Skipping disabled upgrade '{}' because: '{}' ", name,
                         disabled);
                continue;
            }

            // log_info("started loading {}", name);
            auto effects = parse_effects(upgrade["upgrade_effects"]);
            auto prereqs = parse_prereqs(upgrade["prereqs"]);

            auto active_hours = parse_active_hours(upgrade);

            auto required_machines =
                parse_required_machines(name, upgrade["required_machines"]);

            UpgradeLibrary::get().load(
                {
                    .name = name,
                    .icon_name = upgrade.value("icon_name", "upgrade_default"),
                    .flavor_text = upgrade["flavor_text"].get<std::string>(),
                    .description = upgrade["description"].get<std::string>(),
                    .effects = effects,
                    .prereqs = prereqs,
                    .required_machines = required_machines,
                    .duration = upgrade.value("duration", -1),
                    .active_hours = active_hours,
                },
                "INVALID", name.c_str());
        }
    };

    const auto load_upgrade_rounds = [](const nlohmann::json& config_values) {
        for (const auto& value : config_values) {
            const auto name = value.get<std::string>();
            try {
                upgrade_rounds.push_back(magic_enum::enum_cast<UpgradeType>(
                                             name, magic_enum::case_insensitive)
                                             .value());
            } catch (std::exception e) {
                std::cout << ("exception converting upgrade type: {}", e.what())
                          << std::endl;
            }
        }
    };

    load_json_config_file("round_upgrades.json",
                          [&](const nlohmann::json& contents) {
                              load_upgrade_rounds(contents["rounds"]);
                              load_config_values(contents["config"]);
                              load_upgrades(contents["upgrades"]);
                          });
}
