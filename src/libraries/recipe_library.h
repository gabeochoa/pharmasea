#pragma once

#include "../ah.h"
#include "../dataclass/ingredient.h"
#include "../engine/singleton.h"
#include "../entities/entity_type.h"

struct Recipe {
    Drink drink;
    std::string base_name;
    std::string viewer_name;
    std::string icon_name;
    IngredientBitSet ingredients;
    IngredientBitSet prereqs;
    int num_drinks;
    // If we need more replace with a bitset "tags"
    bool requires_upgrade;
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

    [[nodiscard]] auto begin() { return impl.begin(); }
    [[nodiscard]] auto end() { return impl.end(); }
    [[nodiscard]] auto begin() const { return impl.begin(); }
    [[nodiscard]] auto end() const { return impl.end(); }

   private:
    struct RecipeLibraryImpl : afterhours::Library<Recipe> {
        virtual Recipe convert_filename_to_object(const char*,
                                                  const char*) override {
            return Recipe{};
        }

        virtual void unload(Recipe) override {}
    } impl;
};

static std::string get_string_for_ingredient(Ingredient ig) {
    return std::string(magic_enum::enum_name<Ingredient>(ig));
}

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

static const std::string& get_icon_name_for_drink(Drink drink) {
    return RecipeLibrary::get().get(get_string_for_drink(drink)).icon_name;
}
static const std::string& get_model_name_for_drink(Drink drink) {
    return RecipeLibrary::get().get(get_string_for_drink(drink)).base_name;
}

static int get_base_price_for_drink(Drink drink) {
    Recipe recipe = RecipeLibrary::get().get(get_string_for_drink(drink));

    // In the future we could add the price ot the config file, which is why im
    // putting this code here but for now lets just base it on the num
    // ingredients return
    // RecipeLibrary::get().get(get_string_for_drink(drink)).base_price;

    int num_ingredients = (int) recipe.ingredients.count();
    int num_prereqs = (int) recipe.prereqs.count();
    return (num_ingredients * 5) + (num_prereqs * 10);
}

int get_average_unlocked_drink_cost();

static bool needs_upgrade(Drink drink) {
    return RecipeLibrary::get()
        .get(get_string_for_drink(drink))
        .requires_upgrade;
}

struct OrderInfo {
    Drink order;
    int max_pathfind_distance;
    float patience_pct;
};

std::tuple<int, int> get_price_for_order(OrderInfo order_info);
