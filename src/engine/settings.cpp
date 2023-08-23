
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
