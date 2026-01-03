

#pragma once

#include <bitset>

#include "../dataclass/ingredient.h"
#include "../engine/log.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_id.h"
#include "base_component.h"

struct AddsIngredient : public BaseComponent {
    using IngredientFetcherFn =
        std::function<IngredientBitSet(Entity&, Entity&)>;
    using ValidationFn = std::function<bool(const Entity&, const Entity&)>;
    using OnDecrementFn = std::function<void(Entity&)>;

    AddsIngredient() {}
    explicit AddsIngredient(const IngredientFetcherFn& ig) : fetcher(ig) {}

    [[nodiscard]] IngredientBitSet get(Entity& entity) const {
        if (!fetcher) {
            log_error("calling AddsIngredient::fetch() without initializing");
        }
        Entity& parent_entity = EntityHelper::getEnforcedEntityForID(parent);
        return fetcher(parent_entity, entity);
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
        Entity& parent_entity = EntityHelper::getEnforcedEntityForID(parent);
        on_decrement(parent_entity);
    }
    [[nodiscard]] int uses_left() const { return num_uses; }

    [[nodiscard]] bool validate(Entity& entity) const {
        if (!validation) return true;
        Entity& parent_entity = EntityHelper::getEnforcedEntityForID(parent);
        return validation(parent_entity, entity);
    }

    auto& set_parent(EntityID id) {
        parent = id;
        return *this;
    }

   private:
    EntityID parent = entity_id::INVALID;
    IngredientFetcherFn fetcher = nullptr;
    ValidationFn validation = nullptr;
    OnDecrementFn on_decrement = nullptr;
    int num_uses = -1;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.num_uses,                  //
            self.parent                     //
        );
    }
};
