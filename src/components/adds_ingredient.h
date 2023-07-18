

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
    void set_num_uses(int nu) { num_uses = nu; }
    void decrement_uses() { num_uses--; }
    [[nodiscard]] int uses_left() const { return num_uses; }

   private:
    IngredientFetcherFn fetcher;
    int num_uses = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
