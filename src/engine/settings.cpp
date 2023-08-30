
#include "settings.h"

#include "../preload.h"

void Settings::update_language_from_index(int index) {
    // TODO handle exception
    auto li = lang_options[index];

    log_info("Loading translations for {} from {}", li.name, li.filename);

    Preload::get().on_language_change(li.name.c_str(), li.filename.c_str());

    // write to our data so itll be in the save file
    data.lang_name = li.name;
}

void Settings::update_ui_theme(const std::string& s) {
    log_info("updating UI theme to {}", s);
    Preload::get().update_ui_theme(s);

    data.ui_theme = s;
}

void Settings::update_theme_from_index(int index) {
    update_ui_theme(get_ui_theme_options()[index]);
}

std::vector<std::string> Settings::get_ui_theme_options() {
    return Preload::get().ui_theme_options();
}

int Settings::get_ui_theme_selected_index() {
    const auto options = get_ui_theme_options();

    auto it = std::find(options.begin(), options.end(), data.ui_theme);
    return (it != options.end())
               ? static_cast<int>(std::distance(options.begin(), it))
               : 0;  // default to 0
}
