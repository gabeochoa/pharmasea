

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../engine/log.h"
#include "../entity.h"
#include "../entity_helper.h"
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
        OptEntity opt_parent = EntityHelper::getEnforcedEntityForID(parent);
        if (!opt_parent) return IngredientBitSet{};
        return fetcher(opt_parent.asE(), entity);
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
        if (!on_decrement) return;
        OptEntity opt_parent = EntityHelper::getEnforcedEntityForID(parent);
        if (!opt_parent) return;
        on_decrement(opt_parent.asE());
    }
    [[nodiscard]] int uses_left() const { return num_uses; }

    [[nodiscard]] bool validate(Entity& entity) const {
        if (!validation) return true;
        OptEntity opt_parent = EntityHelper::getEnforcedEntityForID(parent);
        if (!opt_parent) return false;
        return validation(opt_parent.asE(), entity);
    }

    // Keep the existing API, but store the handle (id) instead of the pointer.
    auto& set_parent(Entity* p) {
        parent = p ? p->id : EntityID::INVALID;
        return *this;
    }

   private:
    EntityID parent = EntityID::INVALID;
    IngredientFetcherFn fetcher = nullptr;
    ValidationFn validation = nullptr;
    OnDecrementFn on_decrement = nullptr;
    int num_uses = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(num_uses);
        s.value4b(parent);
    }
};
