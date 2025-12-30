
#include "preload.h"

#include <istream>

// Extern declaration for the disable-models flag
extern bool disable_models_flag;
#include <memory>
#include <utility>

#include "afterhours/src/font_helper.h"
#include "config_key_library.h"
#include "dataclass/ingredient.h"
#include "dataclass/settings.h"
#include "engine/font_library.h"
#include "engine/keymap.h"
#include "engine/settings.h"
#include "engine/ui/theme.h"
#include "intro/intro_runner.h"
#include "intro/logo_intro_scene.h"
#include "intro/primary_intro_scene.h"
#include "intro/raylib_intro_scene.h"
#include "magic_enum/magic_enum.hpp"
#include "map_generation/map_generation.h"
#include "recipe_library.h"
#include "strings.h"

// TODO move to a config?
// dataclass/settings.h
std::vector<UpgradeType> upgrade_rounds = {{
    UpgradeType::Drink,
    UpgradeType::Upgrade,
    UpgradeType::Drink,
    UpgradeType::Upgrade,
    UpgradeType::Drink,
    UpgradeType::Drink,
    UpgradeType::Upgrade,
}};

int __WIN_H = 720;
int __WIN_W = 1280;
float DEADZONE = 0.25f;
int LOG_LEVEL = 3;
std::vector<std::string> EXAMPLE_MAP;

namespace strings {
std::map<i18n, std::string> pre_translation;
}  // end namespace strings

#include "translations/translation_en_rev.h"
#include "translations/translation_en_us.h"
#include "translations/translation_es_la.h"
#include "translations/translation_ko_kr.h"

// Store fonts configuration for on-demand loading
static nlohmann::json cached_fonts_config;

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

        // Use afterhours font helper utilities
        int codepointCount = 0;
        int* codepoints =
            raylib::LoadCodepoints(buffer.str().c_str(), &codepointCount);

        int codepointNoDupsCounts = 0;
        int* codepointsNoDups = afterhours::remove_duplicate_codepoints(
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

void Preload::load_fonts(const nlohmann::json& fonts) {
    // Font loading must happen after InitWindow

    const auto& font_object = fonts.get<nlohmann::json::object_t>();
    // Cache fonts config for on-demand loading
    cached_fonts_config = fonts;

    // OPTIMIZATION: Only load the current language's font on startup
    // Previously loaded all fonts, but 99% of the time users don't change
    // languages

    // Get the actual current language from settings
    const std::string current_language = Settings::get().data.lang_name;

    if (font_object.contains(current_language)) {
        std::string font_name = font_object.at(current_language);
        log_info("loading font {} for language {}", font_name,
                 current_language);
        _load_font_from_name(font_name, current_language);
        font = FontLibrary::get().get(current_language);
    } else {
        log_warn(
            "Default language 'en_us' not found in fonts config, using "
            "fallback");
        font = load_karmina_regular();
    }

    // font = load_karmina_regular();

    // NOTE if you go back to files, load fonts from install folder, instead
    // of local path
    //
    // font = LoadFontEx("./resources/fonts/constan.ttf", 96, 0, 0);
}

void Preload::on_language_change(const char* lang_name, const char* fn) {
    // TODO Reset localization and reload from file...
    (void) fn;

    log_info("loading language {}", lang_name);

    const auto _load_language = [lang_name](const auto& pt) {
        for (auto& key : magic_enum::enum_values<strings::i18n>()) {
            if (!pt.contains(key)) {
                log_warn("{} is missing {}", lang_name,
                         magic_enum::enum_name<strings::i18n>(key));
                continue;
            }

            auto value = pt.at(key);
            strings::pre_translation[key] = value;
        }
    };

    if (std::string(lang_name) == std::string("en_us")) {
        _load_language(strings::en_us::pre_translation);
    } else if (std::string(lang_name) == std::string("en_rev")) {
        _load_language(strings::en_rev::pre_translation);
    } else if (std::string(lang_name) == std::string("ko_kr")) {
        _load_language(strings::ko_kr::pre_translation);
    } else if (std::string(lang_name) == std::string("es_la")) {
        _load_language(strings::es_la::pre_translation);
    }

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
        // Try to load the font for this language from cached config
        if (!cached_fonts_config.is_null() &&
            cached_fonts_config.contains(lang_name)) {
            std::string font_name = cached_fonts_config[lang_name];
            log_info("Loading font {} for language {} on demand", font_name,
                     lang_name);
            _load_font_from_name(font_name, lang_name);
        }

        // Check again if we now have the font
        if (!FontLibrary::get().contains(lang_name)) {
            log_warn("Couldnt find/load a font for {}, using en_us instead",
                     lang_name);
            font = FontLibrary::get().get("en_us");
            return;
        }
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
        LOG_LEVEL = contents.value("LOG_LEVEL", 3);
        log_trace("LOG_LEVEL read from file: {}", LOG_LEVEL);

        DEADZONE = contents.value("DEADZONE", 0.25f);

        load_fonts(contents["fonts"]);

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

namespace {
std::vector<std::unique_ptr<IntroScene>> make_intro_scenes(
    const raylib::Font& font, bool show_intro_animations,
    PrimaryIntroScene*& primary_out) {
    std::vector<std::unique_ptr<IntroScene>> scenes;
    if (show_intro_animations) {
        scenes.push_back(std::make_unique<RaylibIntroScene>(font));
        scenes.push_back(std::make_unique<LogoIntroScene>());
    }
    std::unique_ptr<PrimaryIntroScene> primary_scene =
        std::make_unique<PrimaryIntroScene>(font);
    primary_out = primary_scene.get();
    scenes.push_back(std::move(primary_scene));
    return scenes;
}

std::vector<std::unique_ptr<IntroScene>> make_logo_intro_scenes(
    const raylib::Font& font, bool show_intro_animations) {
    std::vector<std::unique_ptr<IntroScene>> scenes;
    if (!show_intro_animations) {
        return scenes;
    }
    scenes.push_back(std::make_unique<RaylibIntroScene>(font));
    scenes.push_back(std::make_unique<LogoIntroScene>());
    return scenes;
}

void run_intro_animations(const raylib::Font& font,
                          bool show_intro_animations) {
    IntroRunner runner(make_logo_intro_scenes(font, show_intro_animations));
    while (!runner.empty()) {
        float dt = raylib::GetFrameTime();
        runner.update(dt, 0.0F);
        if (raylib::IsKeyPressed(raylib::KEY_ESCAPE)) {
            runner.finish_all();
            break;
        }
    }
}

struct LoadingProgress {
    PrimaryIntroScene* primary_scene = nullptr;
    IntroRunner runner;
    int total_units = 0;
    int completed = 0;

    LoadingProgress(const raylib::Font& font)
        : primary_scene(nullptr),
          runner(make_intro_scenes(font, false, primary_scene)),
          total_units(0),
          completed(0) {}

    void render_initial_frame() { runner.update(raylib::GetFrameTime(), 0.0F); }

    void set_total(int total) { total_units = total; }

    void set_status(const std::string& text) {
        if (primary_scene != nullptr) {
            primary_scene->set_status_text(text);
        }
    }

    void tick() {
        if (total_units <= 0) {
            return;
        }
        float progress =
            static_cast<float>(completed) / static_cast<float>(total_units);
        float dt = raylib::GetFrameTime();
        runner.update(dt, progress);
        completed += 1;
    }

    void finish() {
        float dt = raylib::GetFrameTime();
        runner.update(dt, 1.0F);
        runner.finish_all();
    }
};
}  // namespace

Preload::Preload() {
    reload_config();

    log_info("preload: show_raylib_intro={}", SHOW_RAYLIB_INTRO);
    run_intro_animations(this->font, SHOW_RAYLIB_INTRO);

    LoadingProgress progress(this->font);

    // Count work units
    int total_units = 0;

    // shaders
    total_units += 2;  // see load_shaders

    // textures
    {
        int texture_count = 0;
        Files::get().for_resources_in_folder(
            strings::settings::IMAGES, "drinks",
            [&](const std::string&, const std::string&) { texture_count++; });
        Files::get().for_resources_in_folder(
            strings::settings::IMAGES, "external",
            [&](const std::string&, const std::string&) { texture_count++; });
        Files::get().for_resources_in_folder(
            strings::settings::IMAGES, "upgrade",
            [&](const std::string&, const std::string&) { texture_count++; });
        Files::get().for_resources_in_folder(
            strings::settings::IMAGES, "controls/keyboard_default",
            [&](const std::string&, const std::string&) { texture_count++; });
        Files::get().for_resources_in_folder(
            strings::settings::IMAGES, "controls/xbox_default",
            [&](const std::string&, const std::string&) { texture_count++; });
        load_json_config_file("textures.json", [&](const nlohmann::json& c) {
            texture_count += static_cast<int>(c["textures"].size());
        });
        total_units += texture_count;
    }

    // sounds + music
    if (ENABLE_SOUND) {
        int sound_units = 0;
        sound_units += 8;  // fixed loads
        Files::get().for_resources_in_folder(
            strings::settings::SOUNDS, "pa_announcements",
            [&](const std::string&, const std::string&) { sound_units++; });
        total_units += sound_units;
        total_units += 2;  // music tracks
    }

    // models
    {
        int model_units = 0;
        load_json_config_file("models.json", [&](const nlohmann::json& c) {
            model_units = static_cast<int>(c["models"].size());
        });
        total_units += model_units;
    }

    // drink recipes
    {
        int recipe_units = 0;
        load_json_config_file("drinks.json", [&](const nlohmann::json& c) {
            recipe_units = static_cast<int>(c["drinks"].size());
        });
        total_units += recipe_units;
    }

    progress.set_total(total_units);
    progress.render_initial_frame();

    progress.set_status("Loading shaders");
    load_shaders([&]() { progress.tick(); });

    // Load assets sequentially (parallel loading caused thread safety issues
    // with raylib)
    progress.set_status("Loading textures");
    load_textures([&]() { progress.tick(); });

    if (ENABLE_SOUND) {
        ext::init_audio_device();
        progress.set_status("Loading sounds");
        load_sounds([&]() { progress.tick(); });
        progress.set_status("Loading music");
        load_music([&]() { progress.tick(); });
    }

    progress.set_status("Loading models");
    load_models([&]() { progress.tick(); });
    progress.set_status("Loading recipes");
    load_drink_recipes([&]() { progress.tick(); });

    progress.finish();
    completed_preload_once = true;
}

void Preload::load_models(const std::function<void()>& tick) {
    // Check if --disable-models flag was used and reapply it if needed
    extern bool disable_models_flag;
    log_info("load_models: disable_models_flag = {}, ENABLE_MODELS = {}",
             disable_models_flag, ENABLE_MODELS);
    if (disable_models_flag) {
        ENABLE_MODELS = false;
        log_info("load_models: Set ENABLE_MODELS = false due to flag");
    }

    if (!ENABLE_MODELS) {
        log_warn("Skipping Model Loading (--disable-models flag detected)");
        // Still load model metadata for ModelInfoLibrary even when models are
        // disabled
        load_model_metadata_only(tick);
        return;
    }

    struct ModelConfig {
        ModelInfoLibrary::ModelLoadingInfo info;
        bool lazy_load = false;
    };

    std::vector<ModelConfig> modelConfigs;
    load_json_config_file("models.json", [&](const nlohmann::json& contents) {
        const nlohmann::json& models = contents["models"];
        modelConfigs.reserve(models.size());
        for (const nlohmann::json& object : models) {
            ModelConfig config;
            config.info.folder = object["folder"].get<std::string>();
            config.info.filename = object["filename"].get<std::string>();
            config.info.library_name =
                object["library_name"].get<std::string>();
            config.info.size_scale = object["size_scale"].get<float>();
            config.info.position_offset.x =
                object["position_offset"][0].get<float>();
            config.info.position_offset.y =
                object["position_offset"][1].get<float>();
            config.info.position_offset.z =
                object["position_offset"][2].get<float>();
            config.info.rotation_angle = object["rotation_angle"].get<float>();
            config.lazy_load = object.value("lazy_load", false);
            modelConfigs.push_back(config);
        }
    });

    int total = static_cast<int>(modelConfigs.size());

    for (int index = 0; index < total; ++index) {
        const auto& modelConfig = modelConfigs[index];
        const auto& modelInfo = modelConfig.info;

        log_trace("attempting loading {} as {} ", modelInfo.filename,
                  modelInfo.library_name);

        // Check if this model should be lazy loaded
        if (modelConfig.lazy_load) {
            // Register the lazy model with ModelLibrary for on-demand loading
            ModelLibrary::get().register_lazy_model(
                modelInfo.library_name,
                {
                    .folder = modelInfo.folder.c_str(),
                    .filename = modelInfo.filename.c_str(),
                    .libraryname = modelInfo.library_name.c_str(),
                });
            // Still load the metadata but skip the actual model data
            ModelInfoLibrary::get().load({
                .folder = modelInfo.folder,
                .filename = modelInfo.filename,
                .library_name = modelInfo.library_name,
                .size_scale = modelInfo.size_scale,
                .position_offset = modelInfo.position_offset,
                .rotation_angle = modelInfo.rotation_angle,
            });
            // Skip ModelLibrary::get().load() for the actual model data
            continue;
        }

        // Load all models normally
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

        if (tick) {
            tick();
        }
    }

    log_info("Loaded model json successfully, {} models",
             ModelLibrary::get().size());
}

void Preload::load_model_metadata_only(const std::function<void()>& tick) {
    struct ModelConfig {
        ModelInfoLibrary::ModelLoadingInfo info;
        bool lazy_load = false;
    };

    std::vector<ModelConfig> modelConfigs;
    load_json_config_file("models.json", [&](const nlohmann::json& contents) {
        const nlohmann::json& models = contents["models"];
        modelConfigs.reserve(models.size());
        for (const nlohmann::json& object : models) {
            ModelConfig config;
            config.info.folder = object["folder"].get<std::string>();
            config.info.filename = object["filename"].get<std::string>();
            config.info.library_name =
                object["library_name"].get<std::string>();
            config.info.size_scale = object["size_scale"].get<float>();
            config.info.position_offset = vec3{
                object["position_offset"][0].get<float>(),
                object["position_offset"][1].get<float>(),
                object["position_offset"][2].get<float>(),
            };
            config.info.rotation_angle = object["rotation_angle"].get<float>();
            config.lazy_load = object.value("lazy_load", false);
            modelConfigs.push_back(config);

            if (tick) {
                tick();
            }
        }
    });

    // Load metadata for all models (but not the actual model data)
    for (const auto& modelConfig : modelConfigs) {
        const auto& modelInfo = modelConfig.info;

        // Load the metadata into ModelInfoLibrary
        ModelInfoLibrary::get().load({
            .folder = modelInfo.folder,
            .filename = modelInfo.filename,
            .library_name = modelInfo.library_name,
            .size_scale = modelInfo.size_scale,
            .position_offset = modelInfo.position_offset,
            .rotation_angle = modelInfo.rotation_angle,
        });

        // Register as lazy model so get_and_load_if_needed can work if needed
        ModelLibrary::get().register_lazy_model(
            modelInfo.library_name,
            {
                .folder = modelInfo.folder.c_str(),
                .filename = modelInfo.filename.c_str(),
                .libraryname = modelInfo.library_name.c_str(),
            });
    }

    log_info("Loaded model metadata only, {} model infos",
             ModelInfoLibrary::get().size());
}

void Preload::load_drink_recipes(const std::function<void()>& tick) {
    load_json_config_file("drinks.json", [&](const nlohmann::json& contents) {
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
            if (tick) tick();
        }
    });

    log_info("Loaded drink json successfully, {} drinks",
             RecipeLibrary::get().size());
}

void Preload::load_textures(const std::function<void()>& tick) {
    // TODO add a warning for when you are loading two images with the same name
    // because we dont distinguish between folders this is more likely than youd
    // think
    // TODO add support for prepending the folder name to the texture name

    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "drinks",
        [&](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
            if (tick) tick();
        });

    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "external",
        [&](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
            if (tick) tick();
        });

    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "upgrade",
        [&](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
            if (tick) tick();
        });

    // TODO how safe is the path combination here esp for mac vs windows
    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "controls/keyboard_default",
        [&](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
            if (tick) tick();
        });

    // TODO how safe is the path combination here esp for mac vs windows
    Files::get().for_resources_in_folder(
        strings::settings::IMAGES, "controls/xbox_default",
        [&](const std::string& name, const std::string& filename) {
            TextureLibrary::get().load(filename.c_str(), name.c_str());
            if (tick) tick();
        });

    // Now load the one off ones

    load_json_config_file("textures.json", [&](const nlohmann::json& contents) {
        auto textures = contents["textures"];

        for (auto object : textures) {
            auto folder = object["folder"].get<std::string>();
            auto filename = object["filename"].get<std::string>();
            auto library_name = object["library_name"].get<std::string>();

            TextureLibrary::get().load(
                Files::get().fetch_resource_path(folder, filename).c_str(),
                library_name.c_str());

            log_trace("loaded texture {} ", library_name);
            if (tick) tick();
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
            SPAWN_AREA = Rectangle{MAX_SPOT.x - 1, MAX_SPOT.z - 9, 7, 2};
            TRASH_AREA = Rectangle{MAX_SPOT.x - 1, MAX_SPOT.z + 2, 7, 2};

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

void Preload::load_sounds(const std::function<void()>& tick) {
    auto load_and_tick = [&](const char* file, const char* name) {
        SoundLibrary::get().load(
            Files::get()
                .fetch_resource_path(strings::settings::SOUNDS, file)
                .c_str(),
            name);
        if (tick) tick();
    };

    load_and_tick("roblox_oof.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::ROBLOX));
    load_and_tick("vom.wav",
                  strings::sounds::to_name(strings::sounds::SoundId::VOMIT));
    load_and_tick("select.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::SELECT));
    load_and_tick("select.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::CLICK));
    load_and_tick("water.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::WATER));
    load_and_tick("blender.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::BLENDER));
    load_and_tick("solid.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::SOLID));
    load_and_tick("ice.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::ICE));
    load_and_tick("pickup.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::PICKUP));
    load_and_tick("place.ogg",
                  strings::sounds::to_name(strings::sounds::SoundId::PLACE));

    Files::get().for_resources_in_folder(
        strings::settings::SOUNDS, "pa_announcements",
        [&](const std::string& name, const std::string& filename) {
            SoundLibrary::get().load(
                filename.c_str(),
                fmt::format("pa_announcements_{}", name).c_str());
            if (tick) tick();
        });
}

void Preload::load_music(const std::function<void()>& tick) {
    MusicLibrary::get().load(
        Files::get()
            .fetch_resource_path(strings::settings::MUSIC, "jaunt.ogg")
            .c_str(),
        "supermarket");
    if (tick) tick();

    MusicLibrary::get().load(
        Files::get()
            .fetch_resource_path(strings::settings::MUSIC, "theme.ogg")
            .c_str(),
        "theme");
    if (tick) tick();
}

void Preload::load_shaders(const std::function<void()>& tick) {
    const auto screen_vs = Files::get()
                               .fetch_resource_path(strings::settings::SHADERS,
                                                    "screen_quad.vs");

    const std::tuple<const char*, const char*, const char*> shaders[] = {
        // Screen-space shaders (explicit vertex + fragment)
        {strings::settings::SHADERS, "post_processing.fs", "post_processing"},
        {strings::settings::SHADERS, "discard_alpha.fs", "discard_alpha"},
    };

    for (const auto& s : shaders) {
        ShaderLibrary::get().load(
            screen_vs.c_str(),
            Files::get()
                .fetch_resource_path(std::get<0>(s), std::get<1>(s))
                .c_str(),
            std::get<2>(s));
        if (tick) tick();
    }

    // Lighting shader (explicit vertex + fragment)
    ShaderLibrary::get().load(
        Files::get()
            .fetch_resource_path(strings::settings::SHADERS, "lighting.vs")
            .c_str(),
        Files::get()
            .fetch_resource_path(strings::settings::SHADERS, "lighting.fs")
            .c_str(),
        "lighting");
    if (tick) tick();
}

void Preload::load_translations() {
    // TODO :IMPACT: load correct language pack for settings file
    auto path = Files::get().fetch_resource_path(
        strings::settings::TRANSLATIONS, "en_us.mo");
    on_language_change("en_us", path.c_str());
}
