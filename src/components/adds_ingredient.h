

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../engine/log.h"
#include "../entity.h"
#include "base_component.h"

struct AddsIngredient : public BaseComponent {
    using IngredientFetcherFn =
        std::function<IngredientBitSet(Entity&, Entity&)>;
    using ValidationFn = std::function<bool(const Entity&, const Entity&)>;
    using OnDecrementFn = std::function<void(Entity&)>;

    AddsIngredient() {}
    explicit AddsIngredient(const IngredientFetcherFn& ig) : fetcher(ig) {}

    virtual ~AddsIngredient() {}

    [[nodiscard]] IngredientBitSet get(Entity& entity) const {
        if (!fetcher) {
            log_error("calling AddsIngredient::fetch() without initializing");
        }
        return fetcher(*parent, entity);
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
    void decrement_uses() {
        num_uses--;
        if (on_decrement) on_decrement(*parent);
    }
    [[nodiscard]] int uses_left() const { return num_uses; }

    [[nodiscard]] bool validate(Entity& entity) const {
        if (!validation) return true;
        return validation(*parent, entity);
    }

   private:
    IngredientFetcherFn fetcher = nullptr;
    ValidationFn validation = nullptr;
    OnDecrementFn on_decrement = nullptr;
    int num_uses = -1;

   public:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this), num_uses);
    }
};

CEREAL_REGISTER_TYPE(AddsIngredient);
