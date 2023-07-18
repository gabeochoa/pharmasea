

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../engine/log.h"
#include "../entity.h"
#include "base_component.h"

typedef std::function<Ingredient(Entity&)> IngredientFetcherFn;

struct AddsIngredient : public BaseComponent {
    AddsIngredient() {}
    AddsIngredient(IngredientFetcherFn ig) : fetcher(ig) {}

    virtual ~AddsIngredient() {}

    [[nodiscard]] Ingredient get(Entity& entity) const {
        if (!fetcher) {
            log_error("calling AddsIngredient::fetch() without initializing");
        }
        return fetcher(entity);
    }
    void set(IngredientFetcherFn fn) { fetcher = fn; }

   private:
    IngredientFetcherFn fetcher;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
