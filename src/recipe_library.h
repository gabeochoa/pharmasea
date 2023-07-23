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
};

SINGLETON_FWD(RecipeLibrary)
struct RecipeLibrary {
    SINGLETON(RecipeLibrary)

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

static IngredientBitSet get_recipe_for_drink(Drink drink) {
    switch (drink) {
        case LAST_DRINK:
        case coke:
            return RecipeLibrary::get().get("coke").ingredients;
        case rum_and_coke:
            return RecipeLibrary::get().get("rum_and_coke").ingredients;
        case Margarita:
            return recipe::MARGARITA;
        case Cosmo:
            return recipe::COSMO;
        case Mojito:
            return recipe::MOJITO;
        case OldFash:
            return recipe::OLD_FASH;
        case Daiquiri:
            return recipe::DAIQUIRI;
        case PinaColada:
            return recipe::PINA_COLADA;
        case GAndT:
            return recipe::G_AND_T;
        case WhiskeySour:
            return recipe::WHISKEY_SOUR;
        case VodkaTonic:
            return recipe::VODKA_TONIC;
    }
    return RecipeLibrary::get().get("coke").ingredients;
}

// TODO i dont like that this lives here but then the entity_makers owns all the
// 3d models
static std::string get_icon_name_for_drink(Drink drink) {
    switch (drink) {
        case LAST_DRINK:
        case coke:
            return RecipeLibrary::get().get("coke").icon_name;
        case rum_and_coke:
            return RecipeLibrary::get().get("rum_and_coke").icon_name;
        case Margarita:
            return "margarita";
        case Cosmo:
            return "cosmo";
        case Mojito:
            return "mojito";
        case OldFash:
            return "old_fash";
        case Daiquiri:
            return "daiquiri";
        case PinaColada:
            return "pina_colada";
        case GAndT:
            return "g_and_t";
        case WhiskeySour:
            return "whiskey_sour";
        case VodkaTonic:
            return "vodka_tonic";
    }
    return RecipeLibrary::get().get("coke").icon_name;
}
