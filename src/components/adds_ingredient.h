

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../engine/log.h"
#include "../entity.h"
#include "base_component.h"

struct AddsIngredient : public BaseComponent {
    using IngredientFetcherFn =
        std::function<IngredientBitSet(const Entity&, Entity&)>;
    using ValidationFn = std::function<bool(const Entity&, const Entity&)>;
    using OnDecrementFn = std::function<void(Entity&)>;

    AddsIngredient() {}
    explicit AddsIngredient(const IngredientFetcherFn& ig) : fetcher(ig) {}

    virtual ~AddsIngredient() {}

    // NOTE: this component no longer stores an Entity* "parent".
    // Pass the owning entity explicitly to avoid pointer-based state.
    [[nodiscard]] IngredientBitSet get(const Entity& owner, Entity& entity) const {
        if (!fetcher) {
            log_error("calling AddsIngredient::fetch() without initializing");
        }
        return fetcher(owner, entity);
    }
    void set(const IngredientFetcherFn& fn) { fetcher = fn; }
    auto& set_validator(const ValidationFn& fn) {
        validation = fn;
        return *this;
    }
    auto& set_on_decrement(const OnDecrementFn& fn) {
        on_decrement = fn;
        return *this;
    }
    auto& set_num_uses(int nu) {
        num_uses = nu;
        return *this;
    }
    void decrement_uses(Entity& owner) {
        num_uses--;
        if (on_decrement) on_decrement(owner);
    }
    [[nodiscard]] int uses_left() const { return num_uses; }

    [[nodiscard]] bool validate(const Entity& owner, const Entity& entity) const {
        if (!validation) return true;
        return validation(owner, entity);
    }

   private:
    IngredientFetcherFn fetcher = nullptr;
    ValidationFn validation = nullptr;
    OnDecrementFn on_decrement = nullptr;
    int num_uses = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(num_uses);
    }
};
