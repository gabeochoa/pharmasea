#pragma once

#include "dataclass/ingredient.h"
#include "engine/library.h"
#include "engine/singleton.h"

struct Recipe {
    Drink drink;
    std::string base_name;
    std::string viewer_name;
    std::string icon_name;
    IngredientBitSet ingredients;
    IngredientBitSet prereqs;
};

SINGLETON_FWD(RecipeLibrary)
struct RecipeLibrary {
    SINGLETON(RecipeLibrary)

    [[nodiscard]] auto size() { return impl.size(); }

    [[nodiscard]] const Recipe& get(const std::string& name) const {
        return impl.get(name);
    }

    [[nodiscard]] Recipe& get(const std::string& name) {
        return impl.get(name);
    }
    void load(const Recipe& r, const char* filename, const char* name) {
        impl.load(filename, name);

        Recipe& recipe = impl.get(name);
        recipe = r;
    }

    void unload_all() { impl.unload_all(); }

   private:
    struct RecipeLibraryImpl : Library<Recipe> {
        virtual Recipe convert_filename_to_object(const char*,
                                                  const char*) override {
            return Recipe{};
        }

        virtual void unload(Recipe) override {}
    } impl;
};

static std::string get_string_for_drink(Drink drink) {
    return std::string(magic_enum::enum_name<Drink>(drink));
}

static IngredientBitSet get_recipe_for_drink(Drink drink) {
    return RecipeLibrary::get().get(get_string_for_drink(drink)).ingredients;
}

static IngredientBitSet get_req_ingredients_for_drink(Drink drink) {
    auto recip = RecipeLibrary::get().get(get_string_for_drink(drink));
    return recip.ingredients | recip.prereqs;
}

static std::string get_icon_name_for_drink(Drink drink) {
    return RecipeLibrary::get().get(get_string_for_drink(drink)).icon_name;
}
static std::string get_model_name_for_drink(Drink drink) {
    return RecipeLibrary::get().get(get_string_for_drink(drink)).base_name;
}
