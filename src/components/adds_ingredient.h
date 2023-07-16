

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "base_component.h"

struct AddsIngredient : public BaseComponent {
    AddsIngredient() : ingredient(Ingredient::Invalid) {}
    AddsIngredient(Ingredient ig) : ingredient(ig) {}

    virtual ~AddsIngredient() {}

    [[nodiscard]] Ingredient get() const { return ingredient; }
    void set(Ingredient i) { ingredient = i; }

   private:
    Ingredient ingredient;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(ingredient);
    }
};
